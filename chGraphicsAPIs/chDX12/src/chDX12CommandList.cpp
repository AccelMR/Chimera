/************************************************************************/
/**
 * @file chDX12CommandList.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ICommandList.
 */
/************************************************************************/
#include "chDX12CommandList.h"

#include "chDX12API.h"
#include "chDX12Texture.h"
#include "chDX12TextureView.h"

namespace chEngineSDK {
namespace {
struct DX12ResourceState
{
  D3D12_BARRIER_SYNC sync = D3D12_BARRIER_SYNC_NONE;
  D3D12_BARRIER_ACCESS access = D3D12_BARRIER_ACCESS_NO_ACCESS;
  D3D12_BARRIER_LAYOUT layout = D3D12_BARRIER_LAYOUT_UNDEFINED;
};

// Barriers recorded per Barrier call; larger batches are split.
constexpr uint32 kMaxBarriersPerCall = 16;

NODISCARD DX12ResourceState
toDX12State(ResourceState state, bool isDepth)
{
  switch (state) {
  case ResourceState::RenderTarget:
    return {D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_ACCESS_RENDER_TARGET,
            D3D12_BARRIER_LAYOUT_RENDER_TARGET};
  case ResourceState::DepthWrite:
    return {D3D12_BARRIER_SYNC_DEPTH_STENCIL, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
            D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE};
  case ResourceState::DepthRead:
    return {D3D12_BARRIER_SYNC_DEPTH_STENCIL | D3D12_BARRIER_SYNC_ALL_SHADING,
            D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
            D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ};
  case ResourceState::ShaderRead:
    // A depth texture keeps the depth read layout, which shaders can also sample.
    return {D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
            isDepth ? D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ
                    : D3D12_BARRIER_LAYOUT_SHADER_RESOURCE};
  case ResourceState::UnorderedAccess:
    return {D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS};
  case ResourceState::CopySource:
    return {D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_SOURCE,
            D3D12_BARRIER_LAYOUT_COPY_SOURCE};
  case ResourceState::CopyDestination:
    return {D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_DEST,
            D3D12_BARRIER_LAYOUT_COPY_DEST};
  case ResourceState::Present:
    return {D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_ACCESS_NO_ACCESS,
            D3D12_BARRIER_LAYOUT_PRESENT};
  case ResourceState::Undefined:
  default:
    return {};
  }
}

NODISCARD D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE
toBeginningAccess(LoadOp loadOp)
{
  switch (loadOp) {
  case LoadOp::Load:
    return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
  case LoadOp::Clear:
    return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
  case LoadOp::DontCare:
  default:
    return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
  }
}

NODISCARD D3D12_RENDER_PASS_ENDING_ACCESS_TYPE
toEndingAccess(StoreOp storeOp)
{
  return storeOp == StoreOp::Store ? D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE
                                   : D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
}
} // namespace

/*
 */
DX12CommandList::DX12CommandList(ID3D12Device4* device,
                                 ID3D12RootSignature* rootSignature,
                                 ID3D12DescriptorHeap* resourceHeap,
                                 ID3D12DescriptorHeap* samplerHeap)
  : m_rootSignature(rootSignature),
    m_descriptorHeaps{resourceHeap, samplerHeap}
{
  // Made closed, so begin() can reset it like every frame after.
  DX12_CHECK(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                        D3D12_COMMAND_LIST_FLAG_NONE,
                                        IID_ID3D12GraphicsCommandList7,
                                        outPtr(m_commandList)));
}

/*
 */
void
DX12CommandList::begin(ID3D12CommandAllocator* allocator)
{
  DX12_CHECK(m_commandList->Reset(allocator, nullptr));
  // A root signature that indexes the heaps from shaders needs them bound first.
  m_commandList->SetDescriptorHeaps(static_cast<UINT>(m_descriptorHeaps.size()),
                                    m_descriptorHeaps.data());
  m_commandList->SetGraphicsRootSignature(m_rootSignature);
}

/*
 */
void
DX12CommandList::end()
{
  DX12_CHECK(m_commandList->Close());
}

/*
 */
void
DX12CommandList::beginRendering(const RenderingDesc& desc)
{
  CH_ASSERT(desc.colorAttachmentCount <= GraphicsLimits::MAX_COLOR_ATTACHMENTS);

  Array<D3D12_RENDER_PASS_RENDER_TARGET_DESC, GraphicsLimits::MAX_COLOR_ATTACHMENTS>
      colorDescs{};
  for (uint32 i = 0; i < desc.colorAttachmentCount; ++i) {
    const ColorAttachment& attachment = desc.colorAttachments[i];
    const auto* view = static_cast<const DX12TextureView*>(attachment.view);
    CH_ASSERT(view);

    D3D12_RENDER_PASS_RENDER_TARGET_DESC& colorDesc = colorDescs[i];
    colorDesc.cpuDescriptor = view->getRenderTargetHandle();
    colorDesc.BeginningAccess.Type = toBeginningAccess(attachment.loadOp);
    colorDesc.BeginningAccess.Clear.ClearValue.Format =
        chFormatToDxgiFormat(view->getFormat());
    colorDesc.BeginningAccess.Clear.ClearValue.Color[0] = attachment.clearColor.r;
    colorDesc.BeginningAccess.Clear.ClearValue.Color[1] = attachment.clearColor.g;
    colorDesc.BeginningAccess.Clear.ClearValue.Color[2] = attachment.clearColor.b;
    colorDesc.BeginningAccess.Clear.ClearValue.Color[3] = attachment.clearColor.a;
    colorDesc.EndingAccess.Type = toEndingAccess(attachment.storeOp);
  }

  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthDesc{};
  const DepthAttachment& depth = desc.depthAttachment;
  if (depth.view) {
    const auto* view = static_cast<const DX12TextureView*>(depth.view);
    depthDesc.cpuDescriptor = view->getDepthTargetHandle();
    depthDesc.DepthBeginningAccess.Type = toBeginningAccess(depth.loadOp);
    depthDesc.DepthBeginningAccess.Clear.ClearValue.Format =
        chFormatToDxgiFormat(view->getFormat());
    depthDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil = {.Depth = depth.clearDepth,
                                                                    .Stencil = 0};
    depthDesc.DepthEndingAccess.Type = toEndingAccess(depth.storeOp);
    // The engine writes no stencil yet.
    depthDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
    depthDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
  }

  m_commandList->BeginRenderPass(desc.colorAttachmentCount, colorDescs.data(),
                                 depth.view ? &depthDesc : nullptr,
                                 D3D12_RENDER_PASS_FLAG_NONE);
}

/*
 */
void
DX12CommandList::endRendering()
{
  m_commandList->EndRenderPass();
}

/*
 */
void
DX12CommandList::barrier(Span<const TextureBarrier> textureBarriers)
{
  Array<D3D12_TEXTURE_BARRIER, kMaxBarriersPerCall> barriers{};
  uint32 count = 0;

  auto flush = [&]() {
    if (count == 0) {
      return;
    }
    const D3D12_BARRIER_GROUP group{.Type = D3D12_BARRIER_TYPE_TEXTURE,
                                    .NumBarriers = count,
                                    .pTextureBarriers = barriers.data()};
    m_commandList->Barrier(1, &group);
    count = 0;
  };

  for (const TextureBarrier& textureBarrier : textureBarriers) {
    const auto* texture = static_cast<const DX12Texture*>(textureBarrier.texture);
    CH_ASSERT(texture);
    const bool isDepth = FormatUtils::isDepth(texture->getFormat());
    DX12ResourceState before = toDX12State(textureBarrier.before, isDepth);
    const DX12ResourceState after = toDX12State(textureBarrier.after, isDepth);
    // Contents are dropped, but the layout change must still run after earlier work in the
    // destination stages, such as an earlier use of the same target in this list.
    if (textureBarrier.before == ResourceState::Undefined) {
      before.sync = after.sync;
    }

    barriers[count++] = {
        .SyncBefore = before.sync,
        .SyncAfter = after.sync,
        .AccessBefore = before.access,
        .AccessAfter = after.access,
        .LayoutBefore = before.layout,
        .LayoutAfter = after.layout,
        .pResource = texture->getHandle(),
        // 0xFFFFFFFF in the first index covers every mip, layer and plane.
        .Subresources = {.IndexOrFirstMipLevel = 0xFFFFFFFF,
                         .NumMipLevels = 0,
                         .FirstArraySlice = 0,
                         .NumArraySlices = 0,
                         .FirstPlane = 0,
                         .NumPlanes = 0},
        .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE};

    if (count == kMaxBarriersPerCall) {
      flush();
    }
  }
  flush();
}

/*
 */
void
DX12CommandList::bindPipeline(const IPipeline& pipeline)
{
  CH_PARAMETER_UNUSED(pipeline);
  CH_ASSERT(false && "Pipelines are not implemented yet.");
}

/*
 */
void
DX12CommandList::pushConstants(const void* data, uint32 size, uint32 offset)
{
  CH_ASSERT(offset + size <= GraphicsLimits::PUSH_CONSTANTS_SIZE);
  CH_ASSERT(size % 4 == 0 && offset % 4 == 0);
  m_commandList->SetGraphicsRoot32BitConstants(0, size / 4, data, offset / 4);
}

/*
 */
void
DX12CommandList::bindVertexBuffer(const IBuffer& buffer, uint32 binding, uint64 offset)
{
  CH_PARAMETER_UNUSED(buffer);
  CH_PARAMETER_UNUSED(binding);
  CH_PARAMETER_UNUSED(offset);
  CH_ASSERT(false && "Buffers are not implemented yet.");
}

/*
 */
void
DX12CommandList::bindIndexBuffer(const IBuffer& buffer, IndexType indexType, uint64 offset)
{
  CH_PARAMETER_UNUSED(buffer);
  CH_PARAMETER_UNUSED(indexType);
  CH_PARAMETER_UNUSED(offset);
  CH_ASSERT(false && "Buffers are not implemented yet.");
}

/*
 */
void
DX12CommandList::draw(uint32 vertexCount,
                      uint32 instanceCount,
                      uint32 firstVertex,
                      uint32 firstInstance)
{
  m_commandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

/*
 */
void
DX12CommandList::drawIndexed(uint32 indexCount,
                             uint32 instanceCount,
                             uint32 firstIndex,
                             int32 vertexOffset,
                             uint32 firstInstance)
{
  m_commandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset,
                                      firstInstance);
}

/*
 */
void
DX12CommandList::setViewport(float x,
                             float y,
                             float width,
                             float height,
                             float minDepth,
                             float maxDepth)
{
  const D3D12_VIEWPORT viewport{.TopLeftX = x,
                                .TopLeftY = y,
                                .Width = width,
                                .Height = height,
                                .MinDepth = minDepth,
                                .MaxDepth = maxDepth};
  m_commandList->RSSetViewports(1, &viewport);
}

/*
 */
void
DX12CommandList::setScissor(uint32 x, uint32 y, uint32 width, uint32 height)
{
  const D3D12_RECT scissor{.left = static_cast<LONG>(x),
                           .top = static_cast<LONG>(y),
                           .right = static_cast<LONG>(x + width),
                           .bottom = static_cast<LONG>(y + height)};
  m_commandList->RSSetScissorRects(1, &scissor);
}

} // namespace chEngineSDK
