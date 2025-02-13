/************************************************************************/
/**
 * @file chVulkanRenderPass.h
 * @author AccelMR
 * @date 2025/02/02
 *  Vulkan-specific implementation of a GPU RenderPass.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include "chRenderPass.h"

namespace chEngineSDK {

class CH_CORE_EXPORT VulkanRenderPass final : public RenderPass {
 public:
  /**
   *   Default constructor
   */
  VulkanRenderPass() = default;

  VulkanRenderPass(VkRenderPass renderPass) : 
    m_renderPass(renderPass) {}

  /**
   *   Default destructor
   */
  ~VulkanRenderPass();

  void
  _internalInit(const chGPUDesc::RenderPassDesc& desc) override;

  uint32
  _internalGetSubPassCount() const override{ return m_subPassCount; }

  bool
  _internalIsValid() const override;

  FORCEINLINE VkRenderPass
  getRenderPass() const {
    return m_renderPass;
  }
 
  private:
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  uint32 m_subPassCount = 0;
}; 
} // namespace chEngineSDK