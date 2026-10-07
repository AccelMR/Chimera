/************************************************************************/
/**
 * @file chDX12CommandList.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ICommandList.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

#include "chICommandList.h"

namespace chEngineSDK {

/**
 * Direct3D 12 graphics command list. Every pipeline shares one root signature and the two
 * bindless heaps, so begin() sets them once and pushConstants() writes the root constants
 * without a pipeline.
 */
class DX12CommandList : public ICommandList
{
 public:
  DX12CommandList(ID3D12Device4* device,
                  ID3D12RootSignature* rootSignature,
                  ID3D12DescriptorHeap* resourceHeap,
                  ID3D12DescriptorHeap* samplerHeap);
  ~DX12CommandList() override = default;

  DX12CommandList(const DX12CommandList&) = delete;
  DX12CommandList&
  operator=(const DX12CommandList&) = delete;

  /**
   * Opens the list on an allocator the GPU has finished with and sets the root signature.
   */
  void
  begin(ID3D12CommandAllocator* allocator);

  void
  end();

  void
  beginRendering(const RenderingDesc& desc) override;

  void
  endRendering() override;

  void
  barrier(Span<const TextureBarrier> textureBarriers) override;

  void
  bindPipeline(const IPipeline& pipeline) override;

  void
  pushConstants(const void* data, uint32 size, uint32 offset = 0) override;

  void
  bindVertexBuffer(const IBuffer& buffer, uint32 binding = 0, uint64 offset = 0) override;

  void
  bindIndexBuffer(const IBuffer& buffer, IndexType indexType, uint64 offset = 0) override;

  void
  draw(uint32 vertexCount,
       uint32 instanceCount = 1,
       uint32 firstVertex = 0,
       uint32 firstInstance = 0) override;

  void
  drawIndexed(uint32 indexCount,
              uint32 instanceCount = 1,
              uint32 firstIndex = 0,
              int32 vertexOffset = 0,
              uint32 firstInstance = 0) override;

  void
  setViewport(float x,
              float y,
              float width,
              float height,
              float minDepth = 0.0f,
              float maxDepth = 1.0f) override;

  void
  setScissor(uint32 x, uint32 y, uint32 width, uint32 height) override;

  NODISCARD FORCEINLINE ID3D12GraphicsCommandList7*
  getHandle() const
  {
    return m_commandList.Get();
  }

 private:
  ComPtr<ID3D12GraphicsCommandList7> m_commandList;
  ID3D12RootSignature* m_rootSignature = nullptr;
  Array<ID3D12DescriptorHeap*, 2> m_descriptorHeaps{};
};

} // namespace chEngineSDK
