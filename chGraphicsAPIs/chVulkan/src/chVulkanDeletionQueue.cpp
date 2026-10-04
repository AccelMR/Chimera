/************************************************************************/
/**
 * @file chVulkanDeletionQueue.cpp
 * @author AccelMR
 * @date 2026/10/03
 * @brief
 * Deferred destruction of Vulkan objects.
 */
/************************************************************************/
#include "chVulkanDeletionQueue.h"

#include <vk_mem_alloc.h>

namespace chEngineSDK {
namespace {
template<typename HandleType>
FORCEINLINE HandleType
toHandle(uint64 handle)
{
  return reinterpret_cast<HandleType>(static_cast<SIZE_T>(handle));
}
} // namespace

/*
 */
VulkanDeletionQueue::~VulkanDeletionQueue()
{
  CH_ASSERT(m_pending.empty() && m_timeline == VK_NULL_HANDLE);
}

/*
 */
void
VulkanDeletionQueue::initialize(VkDevice device, VmaAllocator allocator)
{
  m_device = device;
  m_allocator = allocator;

  VkSemaphoreTypeCreateInfo typeInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                                     .pNext = nullptr,
                                     .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
                                     .initialValue = 0};
  VkSemaphoreCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                                   .pNext = &typeInfo,
                                   .flags = 0};
  VK_CHECK(vkCreateSemaphore(m_device, &createInfo, nullptr, &m_timeline));
}

/*
 */
void
VulkanDeletionQueue::destroy()
{
  flush();
  if (m_timeline != VK_NULL_HANDLE) {
    vkDestroySemaphore(m_device, m_timeline, nullptr);
    m_timeline = VK_NULL_HANDLE;
  }
}

/*
 */
void
VulkanDeletionQueue::enqueueHandle(VkObjectType type, uint64 handle, VmaAllocation allocation)
{
  if (handle == 0) {
    return;
  }

  LockGuard<Mutex> lock(m_mutex);
  m_pending.push_back({.handle = handle,
                       .allocation = allocation,
                       .releaseValue = m_submittedValue + 1,
                       .type = type});
}

/*
 */
uint64
VulkanDeletionQueue::nextSubmitValue()
{
  LockGuard<Mutex> lock(m_mutex);
  return ++m_submittedValue;
}

/*
 */
void
VulkanDeletionQueue::collect()
{
  LockGuard<Mutex> lock(m_mutex);
  if (m_pending.empty()) {
    return;
  }

  uint64 completedValue = 0;
  VK_CHECK(vkGetSemaphoreCounterValue(m_device, m_timeline, &completedValue));

  SIZE_T kept = 0;
  for (SIZE_T i = 0; i < m_pending.size(); ++i) {
    if (m_pending[i].releaseValue <= completedValue) {
      destroyObject(m_pending[i]);
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
VulkanDeletionQueue::flush()
{
  LockGuard<Mutex> lock(m_mutex);
  for (const PendingObject& object : m_pending) {
    destroyObject(object);
  }
  m_pending.clear();
}

/*
 */
void
VulkanDeletionQueue::destroyObject(const PendingObject& object) const
{
  switch (object.type) {
  case VK_OBJECT_TYPE_BUFFER:
    vmaDestroyBuffer(m_allocator, toHandle<VkBuffer>(object.handle), object.allocation);
    break;
  case VK_OBJECT_TYPE_IMAGE:
    vmaDestroyImage(m_allocator, toHandle<VkImage>(object.handle), object.allocation);
    break;
  case VK_OBJECT_TYPE_IMAGE_VIEW:
    vkDestroyImageView(m_device, toHandle<VkImageView>(object.handle), nullptr);
    break;
  case VK_OBJECT_TYPE_SAMPLER:
    vkDestroySampler(m_device, toHandle<VkSampler>(object.handle), nullptr);
    break;
  case VK_OBJECT_TYPE_SEMAPHORE:
    vkDestroySemaphore(m_device, toHandle<VkSemaphore>(object.handle), nullptr);
    break;
  case VK_OBJECT_TYPE_FENCE:
    vkDestroyFence(m_device, toHandle<VkFence>(object.handle), nullptr);
    break;
  default:
    CH_LOG_ERROR(Vulkan, "Deferred destruction does not support object type {0}",
                 object.type);
    break;
  }
}

} // namespace chEngineSDK
