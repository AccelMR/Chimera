/************************************************************************/
/**
 * @file chRenderGraph.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Records the passes of a frame with the textures they use, and orders their barriers.
 */
/************************************************************************/

#include "chRenderGraph.h"

#include "chICommandList.h"
#include "chITexture.h"
#include "chTransientTexturePool.h"

namespace chEngineSDK {

namespace {
// Two passes that write the same texture in the same state still need a barrier between
// them, so the second one waits for the first; reads in the same state do not.
NODISCARD bool
isWriteState(ResourceState state)
{
  return state == ResourceState::RenderTarget || state == ResourceState::DepthWrite ||
         state == ResourceState::UnorderedAccess || state == ResourceState::CopyDestination;
}
} // namespace

/*
 */
RenderPassBuilder&
RenderPassBuilder::read(RGTextureHandle texture)
{
  const bool isDepth = FormatUtils::isDepth(m_graph.getTextureDesc(texture).format);
  m_graph.addAccess(m_passIndex,
                    {.texture = texture.index,
                     .type = RenderGraph::AccessType::Read,
                     .state = isDepth ? ResourceState::DepthRead : ResourceState::ShaderRead});
  return *this;
}

/*
 */
RenderPassBuilder&
RenderPassBuilder::writeColor(RGTextureHandle texture,
                              LoadOp loadOp,
                              const LinearColor& clearColor)
{
  CH_ASSERT(!FormatUtils::isDepth(m_graph.getTextureDesc(texture).format));
  CH_ASSERT(m_graph.m_passes[m_passIndex].colorCount < GraphicsLimits::MAX_COLOR_ATTACHMENTS);
  m_graph.addAccess(m_passIndex,
                    {.texture = texture.index,
                     .type = RenderGraph::AccessType::Color,
                     .state = ResourceState::RenderTarget,
                     .loadOp = loadOp,
                     .clearColor = clearColor});
  ++m_graph.m_passes[m_passIndex].colorCount;
  return *this;
}

/*
 */
RenderPassBuilder&
RenderPassBuilder::writeDepth(RGTextureHandle texture, LoadOp loadOp, float clearDepth)
{
  CH_ASSERT(FormatUtils::isDepth(m_graph.getTextureDesc(texture).format));
  m_graph.addAccess(m_passIndex,
                    {.texture = texture.index,
                     .type = RenderGraph::AccessType::Depth,
                     .state = ResourceState::DepthWrite,
                     .loadOp = loadOp,
                     .clearDepth = clearDepth});
  return *this;
}

/*
 */
RenderPassBuilder&
RenderPassBuilder::setSideEffect()
{
  m_graph.m_passes[m_passIndex].sideEffect = true;
  return *this;
}

/*
 */
void
RenderGraph::reset()
{
  m_allocator.reset();
  m_textures.clear();
  m_passes.clear();
  m_accesses.clear();
  m_livePasses.clear();
  m_neededTextures.clear();
  m_slots.clear();
  m_compiledPasses.clear();
  m_barriers.clear();
  m_firstFinalBarrier = 0;
  m_compiled = false;
}

/*
 */
RGTextureHandle
RenderGraph::createTexture(StringView name, const RGTextureDesc& desc)
{
  CH_ASSERT(desc.format != Format::Unknown && desc.width > 0 && desc.height > 0);
  m_textures.push_back({.name = name, .desc = desc});
  return {.index = static_cast<uint32>(m_textures.size() - 1)};
}

/*
 */
RGTextureHandle
RenderGraph::importTexture(StringView name,
                           const ITexture& texture,
                           const ITextureView& view,
                           ResourceState initialState,
                           ResourceState finalState)
{
  m_textures.push_back({.name = name,
                        .desc = {.format = texture.getFormat(),
                                 .width = texture.getWidth(),
                                 .height = texture.getHeight(),
                                 .mipLevels = texture.getMipLevels(),
                                 .arrayLayers = texture.getArrayLayers()},
                        .importedTexture = &texture,
                        .importedView = &view,
                        .initialState = initialState,
                        .finalState = finalState});
  return {.index = static_cast<uint32>(m_textures.size() - 1)};
}

/*
 */
RenderPassBuilder
RenderGraph::addPass(StringView name)
{
  m_passes.push_back({.name = name, .firstAccess = static_cast<uint32>(m_accesses.size())});
  return RenderPassBuilder(*this, static_cast<uint32>(m_passes.size() - 1));
}

/*
 */
void
RenderGraph::addAccess(uint32 passIndex, const Access& access)
{
  // Accesses are stored one pass after the other, so a pass is declared before the next
  // one is added.
  CH_ASSERT(passIndex == m_passes.size() - 1);
  CH_ASSERT(access.texture < m_textures.size());

  Pass& pass = m_passes[passIndex];
#if USING(CH_DEBUG_MODE)
  const RGTextureDesc& desc = m_textures[access.texture].desc;
  for (uint32 i = 0; i < pass.accessCount; ++i) {
    const Access& other = m_accesses[pass.firstAccess + i];
    // One texture in two states inside one pass cannot be expressed with barriers.
    CH_ASSERT(other.texture != access.texture);
    // Every attachment of one rendering has the same size.
    CH_ASSERT(access.type == AccessType::Read || other.type == AccessType::Read ||
              (m_textures[other.texture].desc.width == desc.width &&
               m_textures[other.texture].desc.height == desc.height));
  }
#endif

  m_accesses.push_back(access);
  ++pass.accessCount;
}

/*
 */
void
RenderGraph::compile()
{
  cullPasses();
  computeLifetimes();
  assignSlots();
  recordBarriers();
  m_compiled = true;
}

/*
 */
void
RenderGraph::execute(ICommandList& commandList, TransientTexturePool& pool)
{
  CH_ASSERT(m_compiled);

  m_slotEntries.resize(m_slots.size());
  for (SIZE_T i = 0; i < m_slots.size(); ++i) {
    m_slotEntries[i] = pool.acquire(m_slots[i].desc, m_slots[i].usage);
  }

  // Resolved after every acquire, because an acquire can move the pool's entries.
  m_resolvedTextures.resize(m_textures.size());
  m_resolvedViews.resize(m_textures.size());
  for (SIZE_T i = 0; i < m_textures.size(); ++i) {
    const TextureNode& texture = m_textures[i];
    if (isImported(texture)) {
      m_resolvedTextures[i] = texture.importedTexture;
      m_resolvedViews[i] = texture.importedView;
    }
    else if (texture.slot != INVALID_INDEX) {
      const TransientTexturePool::Entry& entry = pool.getEntry(m_slotEntries[texture.slot]);
      m_resolvedTextures[i] = entry.texture.get();
      m_resolvedViews[i] = entry.view.get();
    }
    else {
      m_resolvedTextures[i] = nullptr;
      m_resolvedViews[i] = nullptr;
    }
  }

  for (uint32 compiledIndex = 0; compiledIndex < m_compiledPasses.size(); ++compiledIndex) {
    submitBarriers(commandList, pool, getCompiledPassBarriers(compiledIndex));

    const Pass& pass = m_passes[m_compiledPasses[compiledIndex].pass];
    RenderingDesc renderingDesc{};
    bool isRendering = false;
    for (uint32 i = 0; i < pass.accessCount; ++i) {
      const Access& access = m_accesses[pass.firstAccess + i];
      if (access.type == AccessType::Read) {
        continue;
      }

      const RGTextureDesc& desc = m_textures[access.texture].desc;
      renderingDesc.width = desc.width;
      renderingDesc.height = desc.height;
      isRendering = true;
      if (access.type == AccessType::Color) {
        renderingDesc.colorAttachments[renderingDesc.colorAttachmentCount++] = {
            .view = m_resolvedViews[access.texture],
            .loadOp = access.loadOp,
            .storeOp = access.storeOp,
            .clearColor = access.clearColor};
      }
      else {
        renderingDesc.depthAttachment = {.view = m_resolvedViews[access.texture],
                                         .loadOp = access.loadOp,
                                         .storeOp = access.storeOp,
                                         .clearDepth = access.clearDepth};
      }
    }

    if (isRendering) {
      commandList.beginRendering(renderingDesc);
      commandList.setViewport(0.0f, 0.0f, static_cast<float>(renderingDesc.width),
                              static_cast<float>(renderingDesc.height));
      commandList.setScissor(0, 0, renderingDesc.width, renderingDesc.height);
    }

    if (pass.executor) {
      RenderPassContext context(*this, commandList, renderingDesc.width,
                                renderingDesc.height);
      pass.executor->execute(context);
    }

    if (isRendering) {
      commandList.endRendering();
    }
  }

  submitBarriers(commandList, pool, getFinalBarriers());
}

/*
 */
void
RenderGraph::submitBarriers(ICommandList& commandList,
                            TransientTexturePool& pool,
                            Span<const RGBarrier> barriers)
{
  if (barriers.empty()) {
    return;
  }

  m_textureBarriers.clear();
  for (const RGBarrier& barrier : barriers) {
    const TextureNode& texture = m_textures[barrier.texture.index];
    ResourceState before = barrier.before;
    if (!isImported(texture)) {
      // The pool knows the state the last frame left the texture in, which the compiled
      // barriers cannot (they start every slot from Undefined).
      TransientTexturePool::Entry& entry = pool.getEntry(m_slotEntries[texture.slot]);
      before = entry.state;
      entry.state = barrier.after;
    }
    m_textureBarriers.push_back({.texture = m_resolvedTextures[barrier.texture.index],
                                 .before = before,
                                 .after = barrier.after});
  }
  commandList.barrier(m_textureBarriers);
}

/*
 */
const ITexture&
RenderPassContext::getTexture(RGTextureHandle texture) const
{
  CH_ASSERT(texture.index < m_graph.m_resolvedTextures.size() &&
            m_graph.m_resolvedTextures[texture.index] != nullptr);
  return *m_graph.m_resolvedTextures[texture.index];
}

/*
 */
const ITextureView&
RenderPassContext::getView(RGTextureHandle texture) const
{
  CH_ASSERT(texture.index < m_graph.m_resolvedViews.size() &&
            m_graph.m_resolvedViews[texture.index] != nullptr);
  return *m_graph.m_resolvedViews[texture.index];
}

/*
 */
RGTextureDesc
RenderGraph::getTextureDesc(RGTextureHandle texture) const
{
  CH_ASSERT(texture.index < m_textures.size());
  return m_textures[texture.index].desc;
}

/*
 */
StringView
RenderGraph::getCompiledPassName(uint32 compiledIndex) const
{
  CH_ASSERT(compiledIndex < m_compiledPasses.size());
  return m_passes[m_compiledPasses[compiledIndex].pass].name;
}

/*
 */
Span<const RGBarrier>
RenderGraph::getCompiledPassBarriers(uint32 compiledIndex) const
{
  CH_ASSERT(compiledIndex < m_compiledPasses.size());
  const CompiledPass& compiledPass = m_compiledPasses[compiledIndex];
  return Span<const RGBarrier>(m_barriers.data() + compiledPass.firstBarrier,
                               compiledPass.barrierCount);
}

/*
 */
Span<const RGBarrier>
RenderGraph::getFinalBarriers() const
{
  CH_ASSERT(m_compiled);
  return Span<const RGBarrier>(m_barriers.data() + m_firstFinalBarrier,
                               m_barriers.size() - m_firstFinalBarrier);
}

/*
 */
uint32
RenderGraph::getTextureSlot(RGTextureHandle texture) const
{
  CH_ASSERT(m_compiled && texture.index < m_textures.size());
  return m_textures[texture.index].slot;
}

/*
 */
TextureUsageFlags
RenderGraph::getTextureUsage(RGTextureHandle texture) const
{
  CH_ASSERT(m_compiled && texture.index < m_textures.size());
  return m_textures[texture.index].usage;
}

/*
 */
StoreOp
RenderGraph::getAttachmentStoreOp(uint32 compiledIndex, RGTextureHandle texture) const
{
  CH_ASSERT(compiledIndex < m_compiledPasses.size());
  const Pass& pass = m_passes[m_compiledPasses[compiledIndex].pass];
  for (uint32 i = 0; i < pass.accessCount; ++i) {
    const Access& access = m_accesses[pass.firstAccess + i];
    if (access.texture == texture.index && access.type != AccessType::Read) {
      return access.storeOp;
    }
  }
  CH_ASSERT(false && "The pass does not write this texture as an attachment");
  return StoreOp::DontCare;
}

/*
 */
void
RenderGraph::cullPasses()
{
  // Walks the passes backwards keeping which textures a later pass still needs: imported
  // ones with a final state are needed at the end of the frame. A pass runs when it has a
  // side effect or writes a needed texture; then what it overwrites is no longer needed
  // before it, and what it reads (or loads before writing) is. Whether a later pass needs a
  // texture is also whether the pass that writes it must store it.
  m_neededTextures.resize(m_textures.size());
  for (SIZE_T i = 0; i < m_textures.size(); ++i) {
    const TextureNode& texture = m_textures[i];
    m_neededTextures[i] = isImported(texture) && texture.finalState != ResourceState::Undefined;
  }

  m_livePasses.resize(m_passes.size());
  for (SIZE_T passIndex = m_passes.size(); passIndex-- > 0;) {
    const Pass& pass = m_passes[passIndex];
    Access* accesses = m_accesses.data() + pass.firstAccess;

    bool isLive = pass.sideEffect;
    for (uint32 i = 0; i < pass.accessCount && !isLive; ++i) {
      isLive = accesses[i].type != AccessType::Read && m_neededTextures[accesses[i].texture];
    }
    m_livePasses[passIndex] = isLive;
    if (!isLive) {
      continue;
    }

    for (uint32 i = 0; i < pass.accessCount; ++i) {
      Access& access = accesses[i];
      if (access.type != AccessType::Read) {
        // A side effect pass keeps what it writes even if no pass reads it.
        access.storeOp = (m_neededTextures[access.texture] || pass.sideEffect)
                             ? StoreOp::Store
                             : StoreOp::DontCare;
      }
      m_neededTextures[access.texture] =
          access.type == AccessType::Read || access.loadOp == LoadOp::Load;
    }
  }
}

/*
 */
void
RenderGraph::computeLifetimes()
{
  for (TextureNode& texture : m_textures) {
    texture.usage = TextureUsage::NoneUsage;
    texture.firstPass = INVALID_INDEX;
    texture.lastPass = INVALID_INDEX;
    texture.slot = INVALID_INDEX;
  }

  m_compiledPasses.clear();
  for (uint32 passIndex = 0; passIndex < m_passes.size(); ++passIndex) {
    if (!m_livePasses[passIndex]) {
      continue;
    }

    const uint32 compiledIndex = static_cast<uint32>(m_compiledPasses.size());
    m_compiledPasses.push_back({.pass = passIndex});

    const Pass& pass = m_passes[passIndex];
    for (uint32 i = 0; i < pass.accessCount; ++i) {
      const Access& access = m_accesses[pass.firstAccess + i];
      TextureNode& texture = m_textures[access.texture];
      if (texture.firstPass == INVALID_INDEX) {
        texture.firstPass = compiledIndex;
      }
      texture.lastPass = compiledIndex;

      switch (access.type) {
        case AccessType::Read:
          texture.usage |= TextureUsage::Sampled;
          break;
        case AccessType::Color:
          texture.usage |= TextureUsage::ColorAttachment;
          break;
        case AccessType::Depth:
          texture.usage |= TextureUsage::DepthStencil;
          break;
      }
    }
  }
}

/*
 */
void
RenderGraph::assignSlots()
{
  m_slots.clear();
  for (uint32 compiledIndex = 0; compiledIndex < m_compiledPasses.size(); ++compiledIndex) {
    const Pass& pass = m_passes[m_compiledPasses[compiledIndex].pass];
    const Access* accesses = m_accesses.data() + pass.firstAccess;

    for (uint32 i = 0; i < pass.accessCount; ++i) {
      TextureNode& texture = m_textures[accesses[i].texture];
      if (isImported(texture) || texture.firstPass != compiledIndex) {
        continue;
      }

      uint32 slotIndex = 0;
      for (; slotIndex < m_slots.size(); ++slotIndex) {
        const Slot& slot = m_slots[slotIndex];
        if (!slot.inUse && slot.desc == texture.desc && slot.usage == texture.usage) {
          break;
        }
      }
      if (slotIndex == m_slots.size()) {
        m_slots.push_back({.desc = texture.desc, .usage = texture.usage});
      }
      m_slots[slotIndex].inUse = true;
      texture.slot = slotIndex;
    }

    // Freed only after every texture of the pass has its slot, so a texture that dies here
    // never shares a slot with one that starts here.
    for (uint32 i = 0; i < pass.accessCount; ++i) {
      const TextureNode& texture = m_textures[accesses[i].texture];
      if (!isImported(texture) && texture.lastPass == compiledIndex) {
        m_slots[texture.slot].inUse = false;
      }
    }
  }
}

/*
 */
void
RenderGraph::recordBarriers()
{
  m_barriers.clear();
  for (TextureNode& texture : m_textures) {
    texture.state = texture.initialState;
  }
  // A slot's first texture of the frame starts from Undefined here; whoever runs the graph
  // knows the state the slot's real texture was left in by the last frame.
  for (Slot& slot : m_slots) {
    slot.state = ResourceState::Undefined;
  }

  for (CompiledPass& compiledPass : m_compiledPasses) {
    compiledPass.firstBarrier = static_cast<uint32>(m_barriers.size());

    const Pass& pass = m_passes[compiledPass.pass];
    for (uint32 i = 0; i < pass.accessCount; ++i) {
      const Access& access = m_accesses[pass.firstAccess + i];
      ResourceState& state = getTrackedState(m_textures[access.texture]);
      if (state != access.state || isWriteState(access.state)) {
        m_barriers.push_back({.texture = {.index = access.texture},
                              .before = state,
                              .after = access.state});
        state = access.state;
      }
    }

    compiledPass.barrierCount =
        static_cast<uint32>(m_barriers.size()) - compiledPass.firstBarrier;
  }

  m_firstFinalBarrier = static_cast<uint32>(m_barriers.size());
  for (uint32 textureIndex = 0; textureIndex < m_textures.size(); ++textureIndex) {
    const TextureNode& texture = m_textures[textureIndex];
    if (isImported(texture) && texture.finalState != ResourceState::Undefined &&
        texture.state != texture.finalState) {
      m_barriers.push_back({.texture = {.index = textureIndex},
                            .before = texture.state,
                            .after = texture.finalState});
    }
  }
}

/*
 */
ResourceState&
RenderGraph::getTrackedState(TextureNode& texture)
{
  return isImported(texture) ? texture.state : m_slots[texture.slot].state;
}

} // namespace chEngineSDK
