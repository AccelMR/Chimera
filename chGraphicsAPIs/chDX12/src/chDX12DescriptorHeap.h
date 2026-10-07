/************************************************************************/
/**
 * @file chDX12DescriptorHeap.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Fixed size Direct3D 12 descriptor heap that hands out single descriptors.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"
#include "chSTDThreading.h"

namespace chEngineSDK {

/**
 * Descriptor heap of a fixed size where each descriptor is taken and given back by index,
 * so views never allocate heaps of their own. The same class backs the CPU heaps of render
 * targets and depth targets and the shader visible bindless heaps.
 */
class DX12DescriptorHeap
{
 public:
  DX12DescriptorHeap() = default;

  DX12DescriptorHeap(const DX12DescriptorHeap&) = delete;
  DX12DescriptorHeap&
  operator=(const DX12DescriptorHeap&) = delete;

  void
  initialize(ID3D12Device* device,
             D3D12_DESCRIPTOR_HEAP_TYPE type,
             uint32 capacity,
             bool shaderVisible,
             const ANSICHAR* debugName);

  void
  destroy();

  /**
   * Throws when the heap is full.
   */
  NODISCARD uint32
  allocate();

  void
  free(uint32 index);

  NODISCARD FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE
  getCpuHandle(uint32 index) const
  {
    return {m_cpuStart.ptr + static_cast<SIZE_T>(index) * m_descriptorSize};
  }

  NODISCARD FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE
  getGpuHandle(uint32 index) const
  {
    return {m_gpuStart.ptr + static_cast<uint64>(index) * m_descriptorSize};
  }

  NODISCARD FORCEINLINE ID3D12DescriptorHeap*
  getHeap() const
  {
    return m_heap.Get();
  }

 private:
  ComPtr<ID3D12DescriptorHeap> m_heap;
  D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart{};
  D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart{};
  uint32 m_descriptorSize = 0;
  uint32 m_capacity = 0;
  // Indexes below it have been handed out at least once; freed ones go to m_freeIndices.
  uint32 m_nextUnused = 0;
  Vector<uint32> m_freeIndices;
  Mutex m_mutex;
};

} // namespace chEngineSDK
