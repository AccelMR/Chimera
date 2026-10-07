/************************************************************************/
/**
 * @file chDX12DeletionQueue.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Deferred destruction of Direct3D 12 objects.
 */
/************************************************************************/
#include "chDX12DeletionQueue.h"

#include "chDX12API.h"
#include "chDX12DescriptorHeap.h"

namespace chEngineSDK {

/*
 */
DX12DeletionQueue::~DX12DeletionQueue()
{
  CH_ASSERT(m_pending.empty() && !m_fence);
}

/*
 */
void
DX12DeletionQueue::initialize(ID3D12Device* device)
{
  DX12_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence,
                                 outPtr(m_fence)));
  g_dx12API().setDebugName(m_fence.Get(), "Frame Fence");
}

/*
 */
void
DX12DeletionQueue::destroy()
{
  {
    LockGuard<Mutex> lock(m_mutex);
    for (const PendingObject& object : m_pending) {
      release(object);
    }
    m_pending.clear();
  }

  m_fence.Reset();
}

/*
 */
void
DX12DeletionQueue::enqueueObject(IUnknown* object)
{
  if (object == nullptr) {
    return;
  }
  enqueuePending({.object = object});
}

/*
 */
void
DX12DeletionQueue::enqueueDescriptor(DX12DescriptorHeap& heap, uint32 index)
{
  if (index == GraphicsLimits::INVALID_BINDLESS_INDEX) {
    return;
  }
  enqueuePending({.heap = &heap, .descriptorIndex = index});
}

/*
 */
void
DX12DeletionQueue::enqueuePending(PendingObject object)
{
  LockGuard<Mutex> lock(m_mutex);
  object.releaseValue = m_submittedValue + 1;
  m_pending.push_back(object);
}

/*
 */
uint64
DX12DeletionQueue::nextSubmitValue()
{
  LockGuard<Mutex> lock(m_mutex);
  return ++m_submittedValue;
}

/*
 */
void
DX12DeletionQueue::waitForValue(uint64 value)
{
  if (m_fence->GetCompletedValue() >= value) {
    return;
  }
  // A null event makes the call block until the fence reaches the value.
  DX12_CHECK(m_fence->SetEventOnCompletion(value, nullptr));
}

/*
 */
void
DX12DeletionQueue::collect()
{
  LockGuard<Mutex> lock(m_mutex);
  if (m_pending.empty()) {
    return;
  }

  const uint64 completedValue = m_fence->GetCompletedValue();
  SIZE_T kept = 0;
  for (SIZE_T i = 0; i < m_pending.size(); ++i) {
    if (m_pending[i].releaseValue <= completedValue) {
      release(m_pending[i]);
    }
    else {
      m_pending[kept++] = m_pending[i];
    }
  }
  m_pending.resize(kept);
}

/*
 */
void
DX12DeletionQueue::release(const PendingObject& object)
{
  if (object.heap != nullptr) {
    object.heap->free(object.descriptorIndex);
    return;
  }
  object.object->Release();
}

} // namespace chEngineSDK
