/************************************************************************/
/**
 * @file chRenderGraph.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Records the passes of a frame with the textures they use, and orders their barriers.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"
#include "chLinearAllocator.h"
#include "chLinearColor.h"

namespace chEngineSDK {

class RenderGraph;
class RenderPassContext;
class TransientTexturePool;

/**
 * Names a texture of the current frame of a RenderGraph; it means nothing after reset().
 */
struct RGTextureHandle
{
  NODISCARD constexpr bool
  isValid() const
  {
    return index != INVALID;
  }

  bool
  operator==(const RGTextureHandle& other) const = default;

  static constexpr uint32 INVALID = ~0u;

  uint32 index = INVALID;
};

/**
 * A texture the graph makes for the frame. Its usage comes from what the passes do with it.
 */
struct RGTextureDesc
{
  bool
  operator==(const RGTextureDesc& other) const = default;

  Format format = Format::Unknown;
  uint32 width = 0;
  uint32 height = 0;
  uint32 mipLevels = 1;
  uint32 arrayLayers = 1;
  SampleCount samples = SampleCount::Count1;
};

/**
 * One state change the graph records before a pass, or at the end of the frame.
 */
struct RGBarrier
{
  RGTextureHandle texture;
  ResourceState before = ResourceState::Undefined;
  ResourceState after = ResourceState::Undefined;
};

/**
 * The code of a pass. Lives in the graph's frame allocator, so it is destroyed by reset().
 */
class RenderPassExecutor
{
 public:
  virtual void
  execute(RenderPassContext& context) = 0;

 protected:
  ~RenderPassExecutor() = default;
};

/**
 * Declares what one pass reads and writes. A pass that writes color or depth gets its
 * rendering begun and ended by the graph, with the attachments in the order declared. The
 * graph stores an attachment only when a later pass (or the end of the frame) needs it.
 */
class CH_CORE_EXPORT RenderPassBuilder
{
 public:
  /**
   * Sampled in a shader (a depth format is read in the DepthRead state).
   */
  RenderPassBuilder&
  read(RGTextureHandle texture);

  RenderPassBuilder&
  writeColor(RGTextureHandle texture,
             LoadOp loadOp = LoadOp::Clear,
             const LinearColor& clearColor = LinearColor::Black);

  RenderPassBuilder&
  writeDepth(RGTextureHandle texture, LoadOp loadOp = LoadOp::Clear, float clearDepth = 1.0f);

  /**
   * Keeps the pass even when nothing reads what it writes (readbacks, captures).
   */
  RenderPassBuilder&
  setSideEffect();

  /**
   * function is called as function(RenderPassContext&) when the graph runs the pass. It is
   * copied into the graph's frame allocator, so it may capture by value without allocating.
   */
  template<typename Function>
  void
  setExecute(Function&& function);

 private:
  friend class RenderGraph;

  RenderPassBuilder(RenderGraph& graph, uint32 passIndex) noexcept
    : m_graph(graph),
      m_passIndex(passIndex)
  {}

  RenderGraph& m_graph;
  uint32 m_passIndex;
};

/**
 * Exists so the passes of a frame only say what they read and write: the graph drops the
 * passes whose results nobody uses, gives each texture a slot that other textures reuse
 * once it is dead, and records the barriers between the passes.
 *
 * It is built again every frame: reset(), create and import textures, add passes in the
 * order they run, compile(), then run. Its storage is kept between frames, so after the
 * first ones building and compiling do not allocate. Names must outlive the frame (string
 * literals do).
 *
 * The compiled results (passes, barriers, slots) can be read without a GPU, which is how
 * the tests check them.
 */
class CH_CORE_EXPORT RenderGraph
{
 public:
  RenderGraph() = default;

  RenderGraph(const RenderGraph&) = delete;

  RenderGraph&
  operator=(const RenderGraph&) = delete;

  /**
   * Forgets the passes and textures of the last frame.
   */
  void
  reset();

  NODISCARD RGTextureHandle
  createTexture(StringView name, const RGTextureDesc& desc);

  /**
   * A texture that lives outside the graph (the swap chain image, a history buffer). It is
   * in initialState when the frame starts and is left in finalState when it ends;
   * a finalState of Undefined means nothing after the frame needs its contents, so the
   * passes that only write it can be dropped.
   */
  NODISCARD RGTextureHandle
  importTexture(StringView name,
                const ITexture& texture,
                const ITextureView& view,
                ResourceState initialState,
                ResourceState finalState);

  NODISCARD RenderPassBuilder
  addPass(StringView name);

  void
  compile();

  /**
   * Records the compiled passes into the command list: the barriers, the rendering of the
   * passes that write color or depth (with a viewport and scissor over the whole target),
   * and the code of each pass. Created textures come from pool, which must outlive the
   * frames in flight.
   */
  void
  execute(ICommandList& commandList, TransientTexturePool& pool);

  /**
   * A copy, because creating a texture can move the ones already created.
   */
  NODISCARD RGTextureDesc
  getTextureDesc(RGTextureHandle texture) const;

  NODISCARD uint32
  getCompiledPassCount() const
  {
    return static_cast<uint32>(m_compiledPasses.size());
  }

  NODISCARD StringView
  getCompiledPassName(uint32 compiledIndex) const;

  NODISCARD Span<const RGBarrier>
  getCompiledPassBarriers(uint32 compiledIndex) const;

  /**
   * The barriers that leave every imported texture in its final state.
   */
  NODISCARD Span<const RGBarrier>
  getFinalBarriers() const;

  NODISCARD uint32
  getTransientSlotCount() const
  {
    return static_cast<uint32>(m_slots.size());
  }

  /**
   * Slot a created texture got; INVALID_INDEX for an imported texture or one no pass uses.
   */
  NODISCARD uint32
  getTextureSlot(RGTextureHandle texture) const;

  NODISCARD TextureUsageFlags
  getTextureUsage(RGTextureHandle texture) const;

  /**
   * Store operation compile() chose for a texture a compiled pass writes as an attachment.
   */
  NODISCARD StoreOp
  getAttachmentStoreOp(uint32 compiledIndex, RGTextureHandle texture) const;

  static constexpr uint32 INVALID_INDEX = ~0u;

 private:
  friend class RenderPassBuilder;
  friend class RenderPassContext;

  enum class AccessType : uint8
  {
    Read,
    Color,
    Depth
  };

  struct Access
  {
    uint32 texture = INVALID_INDEX;
    AccessType type = AccessType::Read;
    ResourceState state = ResourceState::Undefined;
    LoadOp loadOp = LoadOp::Load;
    // Chosen by compile().
    StoreOp storeOp = StoreOp::Store;
    LinearColor clearColor = LinearColor::Black;
    float clearDepth = 1.0f;
  };

  struct Pass
  {
    StringView name;
    uint32 firstAccess = 0;
    uint32 accessCount = 0;
    uint32 colorCount = 0;
    RenderPassExecutor* executor = nullptr;
    bool sideEffect = false;
  };

  struct TextureNode
  {
    StringView name;
    RGTextureDesc desc;
    const ITexture* importedTexture = nullptr;
    const ITextureView* importedView = nullptr;
    ResourceState initialState = ResourceState::Undefined;
    ResourceState finalState = ResourceState::Undefined;
    // Filled by compile().
    TextureUsageFlags usage = TextureUsage::NoneUsage;
    uint32 firstPass = INVALID_INDEX;
    uint32 lastPass = INVALID_INDEX;
    uint32 slot = INVALID_INDEX;
    ResourceState state = ResourceState::Undefined;
  };

  /**
   * Room for one created texture at a time; textures with the same description and usage
   * and lifetimes that do not overlap share it.
   */
  struct Slot
  {
    RGTextureDesc desc;
    TextureUsageFlags usage = TextureUsage::NoneUsage;
    bool inUse = false;
    ResourceState state = ResourceState::Undefined;
  };

  struct CompiledPass
  {
    uint32 pass = INVALID_INDEX;
    uint32 firstBarrier = 0;
    uint32 barrierCount = 0;
  };

  void
  addAccess(uint32 passIndex, const Access& access);

  NODISCARD bool
  isImported(const TextureNode& texture) const
  {
    return texture.importedTexture != nullptr;
  }

  void
  cullPasses();

  void
  computeLifetimes();

  void
  assignSlots();

  void
  recordBarriers();

  NODISCARD ResourceState&
  getTrackedState(TextureNode& texture);

  void
  submitBarriers(ICommandList& commandList,
                 TransientTexturePool& pool,
                 Span<const RGBarrier> barriers);

  template<typename Function>
  class LambdaPassExecutor final : public RenderPassExecutor
  {
   public:
    explicit LambdaPassExecutor(Function&& function)
      : m_function(std::move(function))
    {}

    explicit LambdaPassExecutor(const Function& function)
      : m_function(function)
    {}

    void
    execute(RenderPassContext& context) override
    {
      m_function(context);
    }

   private:
    Function m_function;
  };

  LinearAllocator m_allocator;
  Vector<TextureNode> m_textures;
  Vector<Pass> m_passes;
  Vector<Access> m_accesses;
  // One per pass, filled by compile(); 1 when the pass runs.
  Vector<uint8> m_livePasses;
  // One per texture, used by compile() while it walks the passes backwards.
  Vector<uint8> m_neededTextures;
  Vector<Slot> m_slots;
  Vector<CompiledPass> m_compiledPasses;
  Vector<RGBarrier> m_barriers;
  uint32 m_firstFinalBarrier = 0;
  bool m_compiled = false;

  // Filled by execute().
  Vector<uint32> m_slotEntries;
  Vector<const ITexture*> m_resolvedTextures;
  Vector<const ITextureView*> m_resolvedViews;
  Vector<TextureBarrier> m_textureBarriers;
};

/**
 * What the code of a pass gets while the graph runs it.
 */
class CH_CORE_EXPORT RenderPassContext
{
 public:
  NODISCARD FORCEINLINE ICommandList&
  getCommandList() const
  {
    return m_commandList;
  }

  NODISCARD const ITexture&
  getTexture(RGTextureHandle texture) const;

  NODISCARD const ITextureView&
  getView(RGTextureHandle texture) const;

  /**
   * Size of the pass's rendering; 0 for a pass that writes no color or depth.
   */
  NODISCARD FORCEINLINE uint32
  getWidth() const
  {
    return m_width;
  }

  NODISCARD FORCEINLINE uint32
  getHeight() const
  {
    return m_height;
  }

 private:
  friend class RenderGraph;

  RenderPassContext(const RenderGraph& graph,
                    ICommandList& commandList,
                    uint32 width,
                    uint32 height) noexcept
    : m_graph(graph),
      m_commandList(commandList),
      m_width(width),
      m_height(height)
  {}

  const RenderGraph& m_graph;
  ICommandList& m_commandList;
  uint32 m_width;
  uint32 m_height;
};

/*
 */
template<typename Function>
void
RenderPassBuilder::setExecute(Function&& function)
{
  using Executor = RenderGraph::LambdaPassExecutor<std::decay_t<Function>>;
  m_graph.m_passes[m_passIndex].executor =
      m_graph.m_allocator.create<Executor>(std::forward<Function>(function));
}

} // namespace chEngineSDK
