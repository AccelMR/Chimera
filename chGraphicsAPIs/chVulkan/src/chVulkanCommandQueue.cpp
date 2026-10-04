/************************************************************************/
/**
 * @file chVulkanCommandQueue.cpp
 * @author AccelMR
 * @date 2025/04/10
 * @brief
 * Vulkan command queue implementation.
 * This file contains the implementation of the command queue
 * interface for Vulkan.
 */
/************************************************************************/

#include "chVulkanCommandQueue.h"

#include "chVulkanAPI.h"
#include "chVulkanCommandBuffer.h"
#include "chVulkanSynchronization.h"

namespace chEngineSDK {
namespace {
// Upper bounds for one submit, so its arrays live on the stack.
constexpr uint32 kMaxSubmitCommandBuffers = 8;
constexpr uint32 kMaxSubmitWaitSemaphores = 8;
// One more signal slot for the deletion queue timeline.
constexpr uint32 kMaxSubmitSignalSemaphores = 9;
} // namespace

/*
 */
VulkanCommandQueue::VulkanCommandQueue(VkQueue queue, QueueType queueType)
  : m_queue(queue),
    m_queueType(queueType)
{
  CH_ASSERT(m_queue != VK_NULL_HANDLE);
}

/*
 */
void
VulkanCommandQueue::submit(const SubmitInfo& submitInfo, const SPtr<IFence>& fence)
{
  const SIZE_T commandBufferCount = submitInfo.commandBuffers.size();
  const SIZE_T waitCount = submitInfo.waitSemaphores.size();
  const SIZE_T signalCount = submitInfo.signalSemaphores.size();
  CH_ASSERT(commandBufferCount <= kMaxSubmitCommandBuffers);
  CH_ASSERT(waitCount <= kMaxSubmitWaitSemaphores);
  CH_ASSERT(signalCount < kMaxSubmitSignalSemaphores);
  CH_ASSERT(submitInfo.waitStages.size() == waitCount);

  Array<VkCommandBufferSubmitInfo, kMaxSubmitCommandBuffers> commandBuffers{};
  for (SIZE_T i = 0; i < commandBufferCount; ++i) {
    const auto* commandBuffer =
        static_cast<const VulkanCommandBuffer*>(submitInfo.commandBuffers[i].get());
    commandBuffers[i] = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                         .pNext = nullptr,
                         .commandBuffer = commandBuffer->getHandle(),
                         .deviceMask = 0};
  }

  Array<VkSemaphoreSubmitInfo, kMaxSubmitWaitSemaphores> waits{};
  for (SIZE_T i = 0; i < waitCount; ++i) {
    const auto* semaphore =
        static_cast<const VulkanSemaphore*>(submitInfo.waitSemaphores[i].get());
    waits[i] = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = semaphore->getHandle(),
                .value = 0,
                .stageMask = pipelineStageToVkPipelineStage(submitInfo.waitStages[i]),
                .deviceIndex = 0};
  }

  Array<VkSemaphoreSubmitInfo, kMaxSubmitSignalSemaphores> signals{};
  for (SIZE_T i = 0; i < signalCount; ++i) {
    const auto* semaphore =
        static_cast<const VulkanSemaphore*>(submitInfo.signalSemaphores[i].get());
    signals[i] = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                  .pNext = nullptr,
                  .semaphore = semaphore->getHandle(),
                  .value = 0,
                  .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                  .deviceIndex = 0};
  }

  VulkanDeletionQueue& deletionQueue = g_vulkanAPI().getDeletionQueue();
  signals[signalCount] = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                          .pNext = nullptr,
                          .semaphore = deletionQueue.getTimeline(),
                          .value = deletionQueue.nextSubmitValue(),
                          .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                          .deviceIndex = 0};

  const VkSubmitInfo2 vkSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .pNext = nullptr,
      .flags = 0,
      .waitSemaphoreInfoCount = static_cast<uint32>(waitCount),
      .pWaitSemaphoreInfos = waits.data(),
      .commandBufferInfoCount = static_cast<uint32>(commandBufferCount),
      .pCommandBufferInfos = commandBuffers.data(),
      .signalSemaphoreInfoCount = static_cast<uint32>(signalCount + 1),
      .pSignalSemaphoreInfos = signals.data()};

  VkFence vkFence = VK_NULL_HANDLE;
  if (fence) {
    vkFence = static_cast<const VulkanFence*>(fence.get())->getHandle();
  }

  VK_CHECK(vkQueueSubmit2(m_queue, 1, &vkSubmitInfo, vkFence));

  deletionQueue.collect();
}

/*
 */
void
VulkanCommandQueue::waitIdle()
{
  VK_CHECK(vkQueueWaitIdle(m_queue));
  g_vulkanAPI().getDeletionQueue().collect();
}
} // namespace chEngineSDK
