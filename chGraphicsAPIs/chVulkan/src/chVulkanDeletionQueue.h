/************************************************************************/
/**
 * @file chVulkanDeletionQueue.h
 * @author AccelMR
 * @date 2026/10/03
 * @brief
 * Deferred destruction of Vulkan objects.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chSTDThreading.h"

namespace chEngineSDK {

/**
 * Frees Vulkan objects once the GPU has finished every submission that could still use
 * them, so releasing a resource never stalls the CPU. Every submit to the graphics queue
 * signals the next value of a timeline semaphore; an object released now waits for the
 * submit after the last one, which also covers submits made outside the engine (ImGui).
 */
class VulkanDeletionQueue
{
 public:
  VulkanDeletionQueue() = default;
  ~VulkanDeletionQueue();

  VulkanDeletionQueue(const VulkanDeletionQueue&) = delete;
  VulkanDeletionQueue&
  operator=(const VulkanDeletionQueue&) = delete;

  void
  initialize(VkDevice device, VmaAllocator allocator);

  /**
   * Frees everything still pending and the timeline. The device must be idle.
   */
  void
  destroy();

  /**
   * Queues an object, plus the VMA allocation of a buffer or image.
   */
  template<typename HandleType>
  void
  enqueue(VkObjectType type, HandleType handle, VmaAllocation allocation = nullptr)
  {
    enqueueHandle(type, reinterpret_cast<uint64>(handle), allocation);
  }

  /**
   * Value the next graphics submit must signal on the timeline.
   */
  NODISCARD uint64
  nextSubmitValue();

  NODISCARD FORCEINLINE VkSemaphore
  getTimeline() const
  {
    return m_timeline;
  }

  /**
   * Frees the objects whose submit has finished.
   */
  void
  collect();

  /**
   * Frees every pending object. The device must be idle.
   */
  void
  flush();

 private:
  struct PendingObject
  {
    uint64 handle = 0;
    VmaAllocation allocation = nullptr;
    uint64 releaseValue = 0;
    VkObjectType type = VK_OBJECT_TYPE_UNKNOWN;
  };

  void
  enqueueHandle(VkObjectType type, uint64 handle, VmaAllocation allocation);

  void
  destroyObject(const PendingObject& object) const;

  VkDevice m_device = VK_NULL_HANDLE;
  VmaAllocator m_allocator = nullptr;
  VkSemaphore m_timeline = VK_NULL_HANDLE;
  uint64 m_submittedValue = 0;
  Vector<PendingObject> m_pending;
  Mutex m_mutex;
};

} // namespace chEngineSDK
