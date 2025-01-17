/************************************************************************/
/**
 * @file chVulkanSwapChain.h
 * @author AccelMR
 * @date 2024/10/20
 *  Vulkan specific implementation for chimera engine.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include "chSwapChain.h"

namespace chEngineSDK {
class VulkanSwapChain final: public SwapChain { 
 protected:
  /*
  *   Default constructor
  */
  VulkanSwapChain() = default;

  /*
  *   Default destructor
  */
   ~VulkanSwapChain();

   friend class GraphicsModuleVulkan;
   friend class VulkanGPUCommandBuffer;
   
   void 
   init(const chGPUDesc::SwapChainDesc& desc) override;
   
   void
   createResources();
   
   void
   _internalResize(uint32 frameCount, uint32 width, uint32 height, FORMAT fromat) override;
   
   void
   _internalPresent(int32 syncInterval, int32 flags);
   
   uint32 
   _internalGetCurrentFrameIndex() const override;
   
   SPtr<Texture>
   getCurrentRenderTarget() const override;

 protected:
   VkSwapchainKHR m_swapchain;
   
   Vector<VkImage> m_images;
   Vector<VkImageView> m_imageViews;
   Vector<VkFramebuffer> m_framebuffers;
};
}