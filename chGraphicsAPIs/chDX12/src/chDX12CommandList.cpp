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
#include "chDX12Buffer.h"
#include "chDX12Pipeline.h"
#include "chDX12Texture.h"
#include "chDX12TextureView.h"
#include "chMath.h"

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
    // The depth read layout allows no shader access; the generic read layout of the direct
    // queue allows depth tests and sampling together.
    return {D3D12_BARRIER_SYNC_DEPTH_STENCIL | D3D12_BARRIER_SYNC_ALL_SHADING,
            D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ};
  case ResourceState::ShaderRead:
    // A sampled depth texture takes the layout of DepthRead, for the same reason.
    return {D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
            isDepth ? D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ
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

  const D3D12_QUERY_HEAP_DESC queryHeapDesc{
      .Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
      .Count = GraphicsLimits::MAX_TIMESTAMPS_PER_FRAME,
      .NodeMask = 0};
  DX12_CHECK(device->CreateQueryHeap(&queryHeapDesc, IID_ID3D12QueryHeap,
                                     outPtr(m_timestampHeap)));

  const D3D12_HEAP_PROPERTIES readbackHeap{.Type = D3D12_HEAP_TYPE_READBACK,
                                           .CPUPageProperty =
                                               D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                           .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
                                           .CreationNodeMask = 0,
                                           .VisibleNodeMask = 0};
  const D3D12_RESOURCE_DESC readbackDesc{
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
      .Alignment = 0,
      .Width = GraphicsLimits::MAX_TIMESTAMPS_PER_FRAME * sizeof(uint64),
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc = {.Count = 1, .Quality = 0},
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      .Flags = D3D12_RESOURCE_FLAG_NONE};
  DX12_CHECK(device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE,
                                             &readbackDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                             nullptr, IID_ID3D12Resource,
                                             outPtr(m_timestampReadback)));
  void* mapped = nullptr;
  DX12_CHECK(m_timestampReadback->Map(0, nullptr, &mapped));
  m_mappedTimestamps = static_cast<const uint64*>(mapped);
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
  m_commandList->SetComputeRootSignature(m_rootSignature);
  m_pipeline = nullptr;
  m_bComputeBound = false;
  m_boundVertexBuffers = 0;
  m_stats = {};
  m_timestampCount = 0;
}

/*
 */
void
DX12CommandList::end()
{
  if (m_timestampCount > 0) {
    m_commandList->ResolveQueryData(m_timestampHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0,
                                    m_timestampCount, m_timestampReadback.Get(), 0);
  }
  DX12_CHECK(m_commandList->Close());
}

/*
 */
void
DX12CommandList::beginRendering(const RenderingDesc& desc)
{
  CH_ASSERT(desc.colorAttachmentCount <= GraphicsLimits::MAX_COLOR_ATTACHMENTS);
  m_stats.recordRendering();

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
  m_stats.recordBarriers(textureBarriers.size());
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
  const auto& dx12Pipeline = static_cast<const DX12Pipeline&>(pipeline);
  m_commandList->SetPipelineState(dx12Pipeline.getHandle());
  m_stats.recordPipelineBind();
  m_bComputeBound = dx12Pipeline.isCompute();
  if (m_bComputeBound) {
    return;
  }

  m_commandList->IASetPrimitiveTopology(dx12Pipeline.getTopology());
  m_pipeline = &dx12Pipeline;

  for (uint32 binding = 0; binding < MAX_VERTEX_BINDINGS; ++binding) {
    D3D12_VERTEX_BUFFER_VIEW& view = m_vertexBuffers[binding];
    const uint32 stride = dx12Pipeline.getStride(binding);
    if ((m_boundVertexBuffers & (1u << binding)) != 0 && view.StrideInBytes != stride) {
      view.StrideInBytes = stride;
      m_commandList->IASetVertexBuffers(binding, 1, &view);
    }
  }
}

/*
 */
void
DX12CommandList::pushConstants(const void* data, uint32 size, uint32 offset)
{
  CH_ASSERT(offset + size <= GraphicsLimits::PUSH_CONSTANTS_SIZE);
  CH_ASSERT(size % 4 == 0 && offset % 4 == 0);
  // Graphics and compute keep separate root arguments.
  if (m_bComputeBound) {
    m_commandList->SetComputeRoot32BitConstants(0, size / 4, data, offset / 4);
  }
  else {
    m_commandList->SetGraphicsRoot32BitConstants(0, size / 4, data, offset / 4);
  }
}

/*
 */
void
DX12CommandList::dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
{
  CH_ASSERT(m_bComputeBound);
  m_stats.recordDispatch();
  m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

/*
 */
void
DX12CommandList::bindVertexBuffer(const IBuffer& buffer, uint32 binding, uint64 offset)
{
  CH_ASSERT(binding < MAX_VERTEX_BINDINGS);
  const auto& dx12Buffer = static_cast<const DX12Buffer&>(buffer);
  D3D12_VERTEX_BUFFER_VIEW& view = m_vertexBuffers[binding];
  view = {.BufferLocation = dx12Buffer.getGpuAddress() + offset,
          .SizeInBytes = static_cast<UINT>(dx12Buffer.getSize() - offset),
          .StrideInBytes = m_pipeline ? m_pipeline->getStride(binding) : 0};
  m_boundVertexBuffers |= 1u << binding;
  m_commandList->IASetVertexBuffers(binding, 1, &view);
}

/*
 */
void
DX12CommandList::bindIndexBuffer(const IBuffer& buffer, IndexType indexType, uint64 offset)
{
  const auto& dx12Buffer = static_cast<const DX12Buffer&>(buffer);
  const D3D12_INDEX_BUFFER_VIEW view{
      .BufferLocation = dx12Buffer.getGpuAddress() + offset,
      .SizeInBytes = static_cast<UINT>(dx12Buffer.getSize() - offset),
      .Format = indexType == IndexType::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT};
  m_commandList->IASetIndexBuffer(&view);
}

/*
 */
void
DX12CommandList::draw(uint32 vertexCount,
                      uint32 instanceCount,
                      uint32 firstVertex,
                      uint32 firstInstance)
{
  m_stats.recordDraw(vertexCount, instanceCount);
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
  m_stats.recordDraw(indexCount, instanceCount);
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

/*
 */
void
DX12CommandList::writeTimestamp(uint32 index)
{
  CH_ASSERT(index < GraphicsLimits::MAX_TIMESTAMPS_PER_FRAME);
  m_commandList->EndQuery(m_timestampHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index);
  m_timestampCount = Math::max(m_timestampCount, index + 1);
}

/*
 */
uint32
DX12CommandList::readTimestamps(Span<uint64> out) const
{
  const uint32 count = Math::min(m_timestampCount, static_cast<uint32>(out.size()));
  for (uint32 i = 0; i < count; ++i) {
    out[i] = m_mappedTimestamps[i];
  }
  return count;
}

} // namespace chEngineSDK
