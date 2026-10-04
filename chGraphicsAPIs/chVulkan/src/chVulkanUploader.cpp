/************************************************************************/
/**
 * @file chVulkanUploader.cpp
 * @author AccelMR
 * @date 2026/10/04
 * @brief
 * Copies CPU data into GPU-only buffers and textures without waiting for the GPU.
 */
/************************************************************************/
#include "chVulkanUploader.h"

#include <cstring>

#include <vk_mem_alloc.h>

#include "chMath.h"
#include "chVulkanAPI.h"
#include "chVulkanDeletionQueue.h"
#include "chVulkanTexture.h"

namespace chEngineSDK {
namespace {
// A 16K texture has 15 mip levels.
constexpr uint32 kMaxMipLevels = 16;

NODISCARD uint64
alignUp(uint64 value, uint64 alignment)
{
  return (value + alignment - 1) / alignment * alignment;
}

void
recordBarrier(VkCommandBuffer commandBuffer,
              const VkMemoryBarrier2* memoryBarrier,
              const VkImageMemoryBarrier2* imageBarrier)
{
  const VkDependencyInfo dependencyInfo{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .pNext = nullptr,
      .dependencyFlags = 0,
      .memoryBarrierCount = memoryBarrier ? 1u : 0u,
      .pMemoryBarriers = memoryBarrier,
      .bufferMemoryBarrierCount = 0,
      .pBufferMemoryBarriers = nullptr,
      .imageMemoryBarrierCount = imageBarrier ? 1u : 0u,
      .pImageMemoryBarriers = imageBarrier};
  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}
} // namespace

/*
 */
VulkanUploader::~VulkanUploader()
{
  CH_ASSERT(m_ringBuffer == VK_NULL_HANDLE);
}

/*
 */
void
VulkanUploader::initialize(VkDevice device,
                           VmaAllocator allocator,
                           uint32 queueFamilyIndex,
                           VulkanDeletionQueue* deletionQueue)
{
  m_device = device;
  m_allocator = allocator;
  m_deletionQueue = deletionQueue;

  const VkBufferCreateInfo ringInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                    .pNext = nullptr,
                                    .flags = 0,
                                    .size = RING_SIZE,
                                    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                    .queueFamilyIndexCount = 0,
                                    .pQueueFamilyIndices = nullptr};
  VmaAllocationCreateInfo allocationInfo{};
  allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT;
  VmaAllocationInfo allocationResult{};
  VK_CHECK(vmaCreateBuffer(m_allocator, &ringInfo, &allocationInfo, &m_ringBuffer,
                           &m_ringAllocation, &allocationResult));
  m_ringData = static_cast<uint8*>(allocationResult.pMappedData);

  VulkanAPI& vulkanAPI = g_vulkanAPI();
  vulkanAPI.setDebugName(VK_OBJECT_TYPE_BUFFER, m_ringBuffer, "Staging Ring");

  const VkCommandPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                         .pNext = nullptr,
                                         .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                         .queueFamilyIndex = queueFamilyIndex};
  for (uint32 i = 0; i < GraphicsLimits::MAX_FRAMES_IN_FLIGHT; ++i) {
    UploadSlot& slot = m_slots[i];
    VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &slot.commandPool));

    const VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = slot.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1};
    VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &slot.commandBuffer));

    const String name = StringUtils::format("Upload Command List {0}", i);
    vulkanAPI.setDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, slot.commandBuffer, name.c_str());
  }
}

/*
 */
void
VulkanUploader::destroy()
{
  for (UploadSlot& slot : m_slots) {
    // Destroying the pool frees its command buffer.
    if (slot.commandPool != VK_NULL_HANDLE) {
      vkDestroyCommandPool(m_device, slot.commandPool, nullptr);
    }
    slot = {};
  }

  if (m_ringBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(m_allocator, m_ringBuffer, m_ringAllocation);
    m_ringBuffer = VK_NULL_HANDLE;
    m_ringAllocation = nullptr;
    m_ringData = nullptr;
  }
  m_pendingSpans.clear();
  m_isRecording = false;
}

/*
 */
void
VulkanUploader::uploadBuffer(VkBuffer buffer, uint64 offset, const void* data, SIZE_T size)
{
  CH_ASSERT(data != nullptr && size > 0);

  const StagingRegion staging = allocateStaging(size);
  memcpy(staging.mappedData, data, size);
  VK_CHECK(vmaFlushAllocation(m_allocator, staging.allocation, staging.offset,
                              size));

  const VkBufferCopy region{.srcOffset = staging.offset, .dstOffset = offset, .size = size};
  vkCmdCopyBuffer(getCommandBuffer(), staging.buffer, buffer, 1, &region);
}

/*
 */
void
VulkanUploader::uploadTexture(const VulkanTexture& texture, const void* data, SIZE_T size)
{
  CH_ASSERT(data != nullptr && size > 0);
  const Format format = texture.getFormat();
  CH_ASSERT(!FormatUtils::isDepth(format) && "Depth textures cannot be uploaded");
  CH_ASSERT(texture.getMipLevels() <= kMaxMipLevels);

  const uint32 layers = texture.getArrayLayers();
  Array<VkBufferImageCopy, kMaxMipLevels> regions{};
  uint32 regionCount = 0;
  SIZE_T dataOffset = 0;
  for (uint32 mip = 0; mip < texture.getMipLevels(); ++mip) {
    const uint32 width = Math::max(texture.getWidth() >> mip, 1u);
    const uint32 height = Math::max(texture.getHeight() >> mip, 1u);
    const uint32 depth = Math::max(texture.getDepth() >> mip, 1u);
    const SIZE_T mipSize = FormatUtils::getMipSize(format, width, height, depth) * layers;
    if (dataOffset + mipSize > size) {
      break;
    }

    regions[regionCount++] = {.bufferOffset = dataOffset,
                              .bufferRowLength = 0,
                              .bufferImageHeight = 0,
                              .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                   .mipLevel = mip,
                                                   .baseArrayLayer = 0,
                                                   .layerCount = layers},
                              .imageOffset = {0, 0, 0},
                              .imageExtent = {width, height, depth}};
    dataOffset += mipSize;
  }

  if (regionCount == 0) {
    CH_LOG_ERROR(Vulkan, "Texture upload of {0} bytes is smaller than its first mip level",
                 size);
    return;
  }

  const StagingRegion staging = allocateStaging(dataOffset);
  memcpy(staging.mappedData, data, dataOffset);
  VK_CHECK(vmaFlushAllocation(m_allocator, staging.allocation, staging.offset,
                              dataOffset));
  for (uint32 i = 0; i < regionCount; ++i) {
    regions[i].bufferOffset += staging.offset;
  }

  const VkCommandBuffer commandBuffer = getCommandBuffer();
  const VkImageSubresourceRange wholeImage{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                           .baseMipLevel = 0,
                                           .levelCount = VK_REMAINING_MIP_LEVELS,
                                           .baseArrayLayer = 0,
                                           .layerCount = VK_REMAINING_ARRAY_LAYERS};

  // The old contents are replaced, so the image starts from UNDEFINED.
  VkImageMemoryBarrier2 barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                .pNext = nullptr,
                                .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                .srcAccessMask = VK_ACCESS_2_NONE,
                                .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .image = texture.getHandle(),
                                .subresourceRange = wholeImage};
  recordBarrier(commandBuffer, nullptr, &barrier);

  vkCmdCopyBufferToImage(commandBuffer, staging.buffer, texture.getHandle(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionCount, regions.data());

  barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
  barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                         VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                         VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  recordBarrier(commandBuffer, nullptr, &barrier);
}

/*
 */
VkCommandBuffer
VulkanUploader::endRecording(uint64 submitValue)
{
  if (!m_isRecording) {
    return VK_NULL_HANDLE;
  }

  UploadSlot& slot = m_slots[m_slotIndex];

  // The frame's commands come after these copies in the same submit and may read the
  // buffers they wrote.
  const VkMemoryBarrier2 copiesToReads{.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                                       .pNext = nullptr,
                                       .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                       .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                       .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                       .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT};
  recordBarrier(slot.commandBuffer, &copiesToReads, nullptr);
  VK_CHECK(vkEndCommandBuffer(slot.commandBuffer));

  slot.submitValue = submitValue;
  const uint64 lastSpanEnd = m_pendingSpans.empty() ? m_ringTail : m_pendingSpans.back().end;
  if (m_ringHead > lastSpanEnd) {
    m_pendingSpans.push_back({.end = m_ringHead, .submitValue = submitValue});
  }

  m_slotIndex = (m_slotIndex + 1) % GraphicsLimits::MAX_FRAMES_IN_FLIGHT;
  m_isRecording = false;
  return slot.commandBuffer;
}

/*
 */
VulkanUploader::StagingRegion
VulkanUploader::allocateStaging(SIZE_T size)
{
  if (size <= RING_SIZE) {
    releaseFinishedSpans();

    uint64 offset = alignUp(m_ringHead, STAGING_ALIGNMENT);
    // A region never wraps around the end of the ring, so it skips to the start instead.
    if (offset % RING_SIZE + size > RING_SIZE) {
      offset = alignUp(offset, RING_SIZE);
    }
    if (offset + size - m_ringTail <= RING_SIZE) {
      m_ringHead = offset + size;
      const uint64 ringOffset = offset % RING_SIZE;
      return {.buffer = m_ringBuffer,
              .offset = ringOffset,
              .mappedData = m_ringData + ringOffset,
              .allocation = m_ringAllocation};
    }
  }

  // Too big, or the ring is still in use: a buffer of its own, freed after the submit that
  // carries these copies, which is the next one.
  const VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                      .pNext = nullptr,
                                      .flags = 0,
                                      .size = size,
                                      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                      .queueFamilyIndexCount = 0,
                                      .pQueueFamilyIndices = nullptr};
  VmaAllocationCreateInfo allocationInfo{};
  allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT;
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = nullptr;
  VmaAllocationInfo allocationResult{};
  VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &allocationInfo, &buffer, &allocation,
                           &allocationResult));
  m_deletionQueue->enqueue(VK_OBJECT_TYPE_BUFFER, buffer, allocation);

  return {.buffer = buffer,
          .offset = 0,
          .mappedData = allocationResult.pMappedData,
          .allocation = allocation};
}

/*
 */
VkCommandBuffer
VulkanUploader::getCommandBuffer()
{
  UploadSlot& slot = m_slots[m_slotIndex];
  if (m_isRecording) {
    return slot.commandBuffer;
  }

  // The slot's command buffer is recorded again only once its last submit has finished.
  const VkSemaphore timeline = m_deletionQueue->getTimeline();
  const VkSemaphoreWaitInfo waitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
                                     .pNext = nullptr,
                                     .flags = 0,
                                     .semaphoreCount = 1,
                                     .pSemaphores = &timeline,
                                     .pValues = &slot.submitValue};
  VK_CHECK(vkWaitSemaphores(m_device, &waitInfo, UINT64_MAX));
  VK_CHECK(vkResetCommandPool(m_device, slot.commandPool, 0));

  const VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr};
  VK_CHECK(vkBeginCommandBuffer(slot.commandBuffer, &beginInfo));

  // Earlier submits may still read or write what these copies overwrite.
  const VkMemoryBarrier2 earlierWorkToCopies{
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
      .pNext = nullptr,
      .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
      .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
      .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT};
  recordBarrier(slot.commandBuffer, &earlierWorkToCopies, nullptr);

  m_isRecording = true;
  return slot.commandBuffer;
}

/*
 */
void
VulkanUploader::releaseFinishedSpans()
{
  if (m_pendingSpans.empty()) {
    return;
  }

  uint64 completedValue = 0;
  VK_CHECK(vkGetSemaphoreCounterValue(m_device, m_deletionQueue->getTimeline(),
                                      &completedValue));

  SIZE_T finished = 0;
  while (finished < m_pendingSpans.size() &&
         m_pendingSpans[finished].submitValue <= completedValue) {
    m_ringTail = m_pendingSpans[finished].end;
    ++finished;
  }
  m_pendingSpans.erase(m_pendingSpans.begin(),
                       m_pendingSpans.begin() + static_cast<int64>(finished));
}

} // namespace chEngineSDK
