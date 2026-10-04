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

#include <vk_mem_alloc.h>

#include "chVulkanAPI.h"
#include "chVulkanTextureView.h"

namespace chEngineSDK {
namespace {
TextureViewType
toDefaultViewType(TextureType type, uint32 arrayLayers)
{
  switch (type) {
  case TextureType::Texture1D:
    return arrayLayers > 1 ? TextureViewType::View1DArray : TextureViewType::View1D;
  case TextureType::Texture3D:
    return TextureViewType::View3D;
  case TextureType::TextureCube:
    return arrayLayers > 6 ? TextureViewType::ViewCubeArray : TextureViewType::ViewCube;
  case TextureType::Texture2D:
  default:
    return arrayLayers > 1 ? TextureViewType::View2DArray : TextureViewType::View2D;
  }
}
} // namespace

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
    m_usage(createInfo.usage),
    m_ownsTexture(true)
{
  // The initial data reaches the image through a copy.
  if (createInfo.initialData) {
    m_usage |= TextureUsage::TransferDst;
  }

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
      .usage = chTextureUsageToVkImageUsage(m_usage),
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

  if (m_usage.isSet(TextureUsage::Sampled)) {
    m_defaultView = createView({.format = Format::Unknown,
                                .viewType = toDefaultViewType(m_type, m_arrayLayers),
                                .bIsDepthStencil = FormatUtils::isDepth(m_format)});
  }
}

/*
 */
VulkanTexture::~VulkanTexture()
{
  if (!m_ownsTexture) {
    return;
  }

  m_defaultView.reset();
  g_vulkanAPI().getDeletionQueue().enqueue(VK_OBJECT_TYPE_IMAGE, m_image, m_allocation);
}

/*
 */
uint32
VulkanTexture::getBindlessIndex() const
{
  return m_defaultView ? m_defaultView->getBindlessIndex()
                       : GraphicsLimits::INVALID_BINDLESS_INDEX;
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
  CH_ASSERT(m_usage.isSet(TextureUsage::TransferDst));
  g_vulkanAPI().getUploader().uploadTexture(*this, data, size);
}

} // namespace chEngineSDK
