/************************************************************************/
/**
 * @file chVulkanTexture.cpp
 * @author AccelMR
 * @date 2025/04/10
 * @brief
 * Vulkan implementation of ITexture.
 */
/************************************************************************/
#include "chVulkanTexture.h"

#include <cstring>

#include <vk_mem_alloc.h>

#include "chVulkanAPI.h"
#include "chVulkanTextureView.h"

namespace chEngineSDK {
/*
 */
VulkanTexture::VulkanTexture(VkDevice device,
                             VmaAllocator allocator,
                             const TextureCreateInfo& createInfo)
  : m_device(device),
    m_allocator(allocator),
    m_width(createInfo.width),
    m_height(createInfo.height),
    m_depth(createInfo.depth),
    m_mipLevels(createInfo.mipLevels),
    m_arrayLayers(createInfo.arrayLayers),
    m_format(createInfo.format),
    m_type(createInfo.type),
    m_ownsTexture(true)
{
  const VkImageCreateInfo imageInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = chTextureTypeToVkImageType(createInfo.type),
      .format = chFormatToVkFormat(createInfo.format),
      .extent = {.width = createInfo.width,
                 .height = createInfo.height,
                 .depth = createInfo.depth},
      .mipLevels = createInfo.mipLevels,
      .arrayLayers = createInfo.arrayLayers,
      .samples = chSampleCountToVkSampleCount(createInfo.samples),
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = chTextureUsageToVkImageUsage(createInfo.usage),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

  VmaAllocationCreateInfo allocationInfo{};
  allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  // Render targets are big and recreated on resize; VMA advises a block of their own.
  if (createInfo.usage.isSet(TextureUsage::ColorAttachment) ||
      createInfo.usage.isSet(TextureUsage::DepthStencil)) {
    allocationInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  }

  VK_CHECK(vmaCreateImage(m_allocator, &imageInfo, &allocationInfo, &m_image, &m_allocation,
                          nullptr));

  if (createInfo.initialData && createInfo.initialDataSize > 0) {
    uploadData(createInfo.initialData, createInfo.initialDataSize);
  }
}

/*
 */
VulkanTexture::~VulkanTexture()
{
  if (!m_ownsTexture) {
    return;
  }

  g_vulkanAPI().getDeletionQueue().enqueue(VK_OBJECT_TYPE_IMAGE, m_image, m_allocation);
}

/*
 */
NODISCARD SPtr<ITextureView>
VulkanTexture::createView(const TextureViewCreateInfo& createInfo)
{
  return chMakeShared<VulkanTextureView>(m_device, this, createInfo);
}

/*
 */
void
VulkanTexture::uploadData(const void* data, SIZE_T size)
{
  CH_ASSERT(data != nullptr);
  CH_ASSERT(size > 0);

  VulkanAPI& vulkanAPI = g_vulkanAPI();

  const VkBufferCreateInfo stagingInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                       .pNext = nullptr,
                                       .flags = 0,
                                       .size = size,
                                       .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                       .queueFamilyIndexCount = 0,
                                       .pQueueFamilyIndices = nullptr};

  VmaAllocationCreateInfo stagingAllocationInfo{};
  stagingAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
  stagingAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VkBuffer stagingBuffer = VK_NULL_HANDLE;
  VmaAllocation stagingAllocation = nullptr;
  VmaAllocationInfo stagingResult{};
  VK_CHECK(vmaCreateBuffer(m_allocator, &stagingInfo, &stagingAllocationInfo, &stagingBuffer,
                           &stagingAllocation, &stagingResult));

  memcpy(stagingResult.pMappedData, data, size);
  VK_CHECK(vmaFlushAllocation(m_allocator, stagingAllocation, 0, size));

  const VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = vulkanAPI.getGraphicsQueueFamilyIndex()};

  VkCommandPool commandPool = VK_NULL_HANDLE;
  VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool));

  const VkCommandBufferAllocateInfo commandBufferAllocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};

  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  VK_CHECK(vkAllocateCommandBuffers(m_device, &commandBufferAllocInfo, &commandBuffer));

  const VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr};
  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  VkImageMemoryBarrier barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = m_image,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel = 0,
                           .levelCount = m_mipLevels,
                           .baseArrayLayer = 0,
                           .layerCount = m_arrayLayers}};

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);

  const VkBufferImageCopy region{.bufferOffset = 0,
                                 .bufferRowLength = 0,
                                 .bufferImageHeight = 0,
                                 .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                      .mipLevel = 0,
                                                      .baseArrayLayer = 0,
                                                      .layerCount = 1},
                                 .imageOffset = {0, 0, 0},
                                 .imageExtent = {m_width, m_height, 1}};

  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, m_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);

  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  const VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .pNext = nullptr,
                                .waitSemaphoreCount = 0,
                                .pWaitSemaphores = nullptr,
                                .pWaitDstStageMask = nullptr,
                                .commandBufferCount = 1,
                                .pCommandBuffers = &commandBuffer,
                                .signalSemaphoreCount = 0,
                                .pSignalSemaphores = nullptr};

  // Waiting here keeps the staging buffer simple; a staging ring will remove the stall.
  const VkQueue graphicsQueue = vulkanAPI.getGraphicsQueueHandle();
  VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
  VK_CHECK(vkQueueWaitIdle(graphicsQueue));

  vkDestroyCommandPool(m_device, commandPool, nullptr);
  vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
}

} // namespace chEngineSDK
