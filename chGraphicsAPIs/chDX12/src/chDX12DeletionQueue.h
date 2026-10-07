/************************************************************************/
/**
 * @file chDX12DeletionQueue.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Deferred destruction of Direct3D 12 objects.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"
#include "chSTDThreading.h"

namespace chEngineSDK {
class DX12DescriptorHeap;

/**
 * Releases Direct3D 12 objects once the GPU has finished every submission that could still
 * use them, so releasing a resource never stalls the CPU. Every submit to the queue signals
 * the next value of one fence; an object released now waits for the submit after the last
 * one, because commands recorded and not submitted yet may still use it.
 */
class DX12DeletionQueue
{
 public:
  DX12DeletionQueue() = default;
  ~DX12DeletionQueue();

  DX12DeletionQueue(const DX12DeletionQueue&) = delete;
  DX12DeletionQueue&
  operator=(const DX12DeletionQueue&) = delete;

  void
  initialize(ID3D12Device* device);

  /**
   * Releases everything still pending and the fence. The GPU must be idle.
   */
  void
  destroy();

  /**
   * Takes over the reference the pointer holds.
   */
  template<typename T>
  void
  enqueue(ComPtr<T>& object)
  {
    enqueueObject(object.Detach());
  }

  /**
   * Takes over one reference of the object, such as a D3D12MA allocation.
   */
  void
  enqueueObject(IUnknown* object);

  /**
   * Queues a descriptor, so it is reused only once nothing reads it.
   */
  void
  enqueueDescriptor(DX12DescriptorHeap& heap, uint32 index);

  /**
   * Value the next submit must signal on the fence.
   */
  NODISCARD uint64
  nextSubmitValue();

  NODISCARD FORCEINLINE ID3D12Fence*
  getFence() const
  {
    return m_fence.Get();
  }

  void
  waitForValue(uint64 value);

  /**
   * Releases the objects whose submit has finished.
   */
  void
  collect();

 private:
  struct PendingObject
  {
    IUnknown* object = nullptr;
    DX12DescriptorHeap* heap = nullptr;
    uint32 descriptorIndex = 0;
    uint64 releaseValue = 0;
  };

  void
  enqueuePending(PendingObject object);

  static void
  release(const PendingObject& object);

  ComPtr<ID3D12Fence> m_fence;
  uint64 m_submittedValue = 0;
  Vector<PendingObject> m_pending;
  Mutex m_mutex;
};

} // namespace chEngineSDK
