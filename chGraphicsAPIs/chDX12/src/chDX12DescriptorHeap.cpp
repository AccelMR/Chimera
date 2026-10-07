/************************************************************************/
/**
 * @file chDX12DescriptorHeap.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Fixed size Direct3D 12 descriptor heap that hands out single descriptors.
 */
/************************************************************************/
#include "chDX12DescriptorHeap.h"

#include "chDX12API.h"

namespace chEngineSDK {

/*
 */
void
DX12DescriptorHeap::initialize(ID3D12Device* device,
                               D3D12_DESCRIPTOR_HEAP_TYPE type,
                               uint32 capacity,
                               bool shaderVisible,
                               const ANSICHAR* debugName)
{
  const D3D12_DESCRIPTOR_HEAP_FLAGS flags = shaderVisible
                                               ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                               : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  const D3D12_DESCRIPTOR_HEAP_DESC desc{.Type = type,
                                        .NumDescriptors = capacity,
                                        .Flags = flags,
                                        .NodeMask = 0};
  DX12_CHECK(device->CreateDescriptorHeap(&desc, IID_ID3D12DescriptorHeap, outPtr(m_heap)));
  g_dx12API().setDebugName(m_heap.Get(), debugName);

  m_cpuStart = m_heap->GetCPUDescriptorHandleForHeapStart();
  if (shaderVisible) {
    m_gpuStart = m_heap->GetGPUDescriptorHandleForHeapStart();
  }
  m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
  m_capacity = capacity;
  m_freeIndices.reserve(capacity);
}

/*
 */
void
DX12DescriptorHeap::destroy()
{
  m_heap.Reset();
  m_freeIndices.clear();
  m_nextUnused = 0;
  m_capacity = 0;
}

/*
 */
uint32
DX12DescriptorHeap::allocate()
{
  LockGuard<Mutex> lock(m_mutex);
  if (!m_freeIndices.empty()) {
    const uint32 index = m_freeIndices.back();
    m_freeIndices.pop_back();
    return index;
  }

  if (m_nextUnused == m_capacity) {
    CH_EXCEPT(DX12ErrorException,
              StringUtils::format("Descriptor heap full ({0} descriptors).", m_capacity));
  }
  return m_nextUnused++;
}

/*
 */
void
DX12DescriptorHeap::free(uint32 index)
{
  LockGuard<Mutex> lock(m_mutex);
  CH_ASSERT(index < m_nextUnused);
  m_freeIndices.push_back(index);
}

} // namespace chEngineSDK
