/************************************************************************/
/**
 * @file chDX12Buffer.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of IBuffer.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

#include "chIBuffer.h"

namespace chEngineSDK {

/**
 * Direct3D 12 buffer whose memory comes from D3D12MA. Buffers the CPU writes live in the
 * upload heap and stay mapped for their whole life; GpuOnly buffers live in the default
 * heap and are filled through the uploader. Everything is released through the deletion
 * queue.
 */
class DX12Buffer : public IBuffer
{
 public:
  DX12Buffer(D3D12MA::Allocator* allocator, const BufferCreateInfo& createInfo);
  ~DX12Buffer() override;

  DX12Buffer(const DX12Buffer&) = delete;
  DX12Buffer&
  operator=(const DX12Buffer&) = delete;

  NODISCARD SIZE_T
  getSize() const override
  {
    return m_size;
  }

  void
  update(const void* data, SIZE_T size, uint32 offset = 0) override;

  NODISCARD uint32
  getBindlessIndex() const override
  {
    return m_bindlessIndex;
  }

  NODISCARD FORCEINLINE ID3D12Resource*
  getHandle() const
  {
    return m_resource.Get();
  }

  NODISCARD FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS
  getGpuAddress() const
  {
    return m_gpuAddress;
  }

 private:
  void
  createBindlessView(const BufferCreateInfo& createInfo,
                     uint64 allocatedSize,
                     bool isWritable);

  ComPtr<ID3D12Resource> m_resource;
  D3D12MA::Allocation* m_allocation = nullptr;
  void* m_mappedData = nullptr;
  D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress = 0;
  SIZE_T m_size = 0;
  uint32 m_bindlessIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
};

} // namespace chEngineSDK
