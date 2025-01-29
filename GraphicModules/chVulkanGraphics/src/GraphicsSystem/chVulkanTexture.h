/************************************************************************/
/**
 * @file chVulkanTexutre.h
 * @author AccelMR
 * @date 2025/01/28
 *   Vulkan Texture implementation.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include "chTexture.h"

namespace chEngineSDK {
class VulkanTexture final : public Texture
{
 public:

  VulkanTexture(VkDevice device) 
    : m_device(device), m_image(nullptr) {}

  VulkanTexture(VkImage image, VkDevice device) 
    : m_image(image), m_device(device) {}

  ~VulkanTexture() override;

  void
  _internalInit(const chGPUDesc::TextureDesc& desc) override;

 private:
  VkImage m_image;
  VkDevice m_device;
};
} // namespace chEngineSDK

