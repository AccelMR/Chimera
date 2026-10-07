/************************************************************************/
/**
 * @file chVulkanSwapChain.cpp
 * @author AccelMR
 * @date 2025/04/07
 * @details
 * SwapChain implementation for Vulkan.
 */
/************************************************************************/
#include "chVulkanSwapChain.h"

#include "chAlgorithm.h"
#include "chMath.h"
#include "chVulkanAPI.h"

namespace chEngineSDK {
namespace {
NODISCARD SwapChainStatus
toSwapChainStatus(VkResult result, StringView action)
{
  switch (result) {
    case VK_SUCCESS: return SwapChainStatus::Ready;
    case VK_SUBOPTIMAL_KHR: return SwapChainStatus::Suboptimal;
    case VK_ERROR_OUT_OF_DATE_KHR: return SwapChainStatus::OutOfDate;
    default:
      CH_LOG_ERROR(Vulkan, "Failed to {0} swap chain image: {1}", action, result);
      return SwapChainStatus::Failed;
  }
}
} // namespace

/*
*/
VulkanSwapChain::VulkanSwapChain(VkInstance instance,
                                 VkDevice device,
                                 VkPhysicalDevice physicalDevice,
                                 uint32 graphicsFamilyQueueIndex,
                                 uint32 presentFamilyQueueIndex,
                                 const SwapChainDesc& desc)
  : m_instance(instance),
    m_device(device),
    m_physicalDevice(physicalDevice),
    m_graphicsFamilyQueueIndex(graphicsFamilyQueueIndex),
    m_presentFamilyQueueIndex(presentFamilyQueueIndex),
    m_debugName(desc.debugName)
{
  CH_ASSERT(m_instance != VK_NULL_HANDLE);
  CH_ASSERT(m_device != VK_NULL_HANDLE);
  CH_ASSERT(m_physicalDevice != VK_NULL_HANDLE);

  createSurface(desc.window);

  // They do not depend on the images, so they survive every resize.
  for (uint32 i = 0; i < GraphicsLimits::MAX_FRAMES_IN_FLIGHT; ++i) {
    const String name = StringUtils::format("{0} Acquire {1}", m_debugName, i);
    m_acquireSemaphores[i] = createSemaphore(name.c_str());
  }

  create(desc.width, desc.height, desc.vsync);
}

/*
*/
VulkanSwapChain::~VulkanSwapChain()
{
  cleanUpSwapChain();
  for (VkSemaphore& semaphore : m_acquireSemaphores) {
    vkDestroySemaphore(m_device, semaphore, nullptr);
    semaphore = VK_NULL_HANDLE;
  }
  // After the swap chain, which was made on it.
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

/*
*/
SwapChainStatus
VulkanSwapChain::acquireNextImage()
{
  VulkanAPI& vulkanAPI = g_vulkanAPI();
  m_acquireSlot = vulkanAPI.getFrameIndex();

  const VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX,
                                                m_acquireSemaphores[m_acquireSlot],
                                                VK_NULL_HANDLE, &m_currentImageIndex);
  const SwapChainStatus status = toSwapChainStatus(result, "acquire");

  // Only an acquired image signals the semaphore, so only then may the frame wait on it.
  if (status == SwapChainStatus::Ready || status == SwapChainStatus::Suboptimal) {
    vulkanAPI.addFrameSwapChain(*this);
  }
  return status;
}

/*
*/
SwapChainStatus
VulkanSwapChain::present()
{
  const VkSemaphore waitSemaphore = m_presentSemaphores[m_currentImageIndex];
  const VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                     .pNext = nullptr,
                                     .waitSemaphoreCount = 1,
                                     .pWaitSemaphores = &waitSemaphore,
                                     .swapchainCount = 1,
                                     .pSwapchains = &m_swapChain,
                                     .pImageIndices = &m_currentImageIndex,
                                     .pResults = nullptr};

  // The swap chain is not recreated here: the caller also owns resources sized to it and
  // decides when to rebuild them.
  const VkResult result = vkQueuePresentKHR(g_vulkanAPI().getGraphicsQueueHandle(),
                                            &presentInfo);
  return toSwapChainStatus(result, "present");
}

/*
*/
void
VulkanSwapChain::create(uint32 width, uint32 height, bool vsync)
{
  cleanUpSwapChain();
  m_vsync = vsync;

  VkSurfaceCapabilitiesKHR capabilities;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice,
                                                     m_surface,
                                                     &capabilities));

  VkExtent2D swapChainExtent = {};
  if (capabilities.currentExtent.width == UINT32_MAX) {
    swapChainExtent.width = Math::clamp(width,
                                        capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
    swapChainExtent.height = Math::clamp(height,
                                         capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);
  }
  else {
    swapChainExtent = capabilities.currentExtent;
  }

  m_width = swapChainExtent.width;
  m_height = swapChainExtent.height;

  uint32 presentModeCount = 0;
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice,
                                                     m_surface,
                                                     &presentModeCount,
                                                     nullptr));

  Vector<VkPresentModeKHR> presentModes(presentModeCount);
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice,
                                                     m_surface,
                                                     &presentModeCount,
                                                     presentModes.data()));

  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  if (!vsync) {
    // Prefer MAILBOX (triple buffering) if it is available
    if (Algorithm::contains(presentModes, VK_PRESENT_MODE_MAILBOX_KHR)) {
      presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    }
    else if (Algorithm::contains(presentModes, VK_PRESENT_MODE_IMMEDIATE_KHR)) {
      // Fallback to IMMEDIATE (no VSync)
      presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
  }

  m_presentMode = presentMode;

  constexpr uint32 imageDefaultCount = 3; // Triple buffering
  if (capabilities.maxImageCount == 0) {
    m_imageCount = Math::max(imageDefaultCount, capabilities.minImageCount);
  }
  else {
    m_imageCount = Math::clamp(imageDefaultCount,
                               capabilities.minImageCount,
                               capabilities.maxImageCount);
  }

  VkSwapchainCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = nullptr,
    .flags = 0,
    .surface = m_surface,
    .minImageCount = m_imageCount,
    .imageFormat = m_colorFormat,
    .imageColorSpace = m_colorSpace,
    .imageExtent = swapChainExtent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr,
    .preTransform = capabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = presentMode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE
  };

  uint32 queueFamilyIndices[] = {m_graphicsFamilyQueueIndex, m_presentFamilyQueueIndex};
  if (m_graphicsFamilyQueueIndex != m_presentFamilyQueueIndex) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }

  VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain));
  VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imageCount, nullptr));
  m_images.resize(m_imageCount);
  VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imageCount, m_images.data()));

  createImageViews();

  const VulkanAPI& vulkanAPI = g_vulkanAPI();
  vulkanAPI.setDebugName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, m_swapChain, m_debugName.c_str());

  m_textures.reserve(m_imageCount);
  m_textureViews.reserve(m_imageCount);
  m_presentSemaphores.reserve(m_imageCount);
  for (uint32 i = 0; i < m_imageCount; ++i) {
    m_textures.push_back(chMakeUnique<VulkanTexture>(m_device, m_images[i], m_colorFormat,
                                                     m_width, m_height, 1, 1));
    m_textureViews.push_back(chMakeUnique<VulkanTextureView>(
        m_device, m_imageViews[i], m_colorFormat, 0, 1, 0, 1, TextureViewType::View2D));

    const String imageName = StringUtils::format("{0} Image {1}", m_debugName, i);
    vulkanAPI.setDebugName(VK_OBJECT_TYPE_IMAGE, m_images[i], imageName.c_str());
    const String viewName = StringUtils::format("{0} View {1}", m_debugName, i);
    vulkanAPI.setDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, m_imageViews[i], viewName.c_str());
    const String semaphoreName = StringUtils::format("{0} Present {1}", m_debugName, i);
    m_presentSemaphores.push_back(createSemaphore(semaphoreName.c_str()));
  }
  m_currentImageIndex = 0;
}

/*
*/
void
VulkanSwapChain::resize(uint32 width, uint32 height)
{
  // A minimized window reports a 0x0 surface before the window system marks it minimized;
  // a swap chain of that size is invalid, so the current one is kept until it is restored.
  VkSurfaceCapabilitiesKHR capabilities;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface,
                                                     &capabilities));
  if (width == 0 || height == 0 || capabilities.currentExtent.width == 0 ||
      capabilities.currentExtent.height == 0) {
    return;
  }

  create(width, height, m_vsync);
}

/*
*/
void
VulkanSwapChain::createSurface(PlatformDisplay window)
{
  CH_ASSERT(window != nullptr);
#if USING(CH_DISPLAY_SDL3)
  if (!SDL_Vulkan_CreateSurface(window, m_instance, nullptr, &m_surface)) {
    CH_EXCEPT(VulkanErrorException,
              StringUtils::format("Failed to create the surface of {0}: {1}", m_debugName,
                                  SDL_GetError()));
  }
#else
  CH_EXCEPT(VulkanErrorException, "Vulkan surfaces are only made through SDL3.");
#endif // USING(CH_DISPLAY_SDL3)

  // The swap chain presents on the graphics queue.
  VkBool32 presentSupported = VK_FALSE;
  VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, m_graphicsFamilyQueueIndex,
                                                m_surface, &presentSupported));
  if (presentSupported != VK_TRUE) {
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    CH_EXCEPT(VulkanErrorException,
              StringUtils::format("The GPU cannot present to {0}", m_debugName));
  }

  uint32 formatCount = 0;
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount,
                                                nullptr));
  Vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount,
                                                surfaceFormats.data()));
  CH_ASSERT(!surfaceFormats.empty());

  // Every window takes the same format when its surface offers it, so the UI draws all of
  // them with one pipeline.
  VkSurfaceFormatKHR chosen = surfaceFormats[0];
  for (const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats) {
    if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
        surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      chosen = surfaceFormat;
      break;
    }
  }
  m_colorFormat = chosen.format;
  m_colorSpace = chosen.colorSpace;
}

/*
*/
void
VulkanSwapChain::cleanUpSwapChain()
{
  // The images, views and semaphores may still be used by frames in flight.
  vkDeviceWaitIdle(m_device);

  m_textureViews.clear();
  m_textures.clear();

  for (VkSemaphore semaphore : m_presentSemaphores) {
    vkDestroySemaphore(m_device, semaphore, nullptr);
  }
  m_presentSemaphores.clear();

  for (VkImageView imageView : m_imageViews) {
    vkDestroyImageView(m_device, imageView, nullptr);
  }
  m_imageViews.clear();
  m_images.clear();

  if (m_swapChain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    m_swapChain = VK_NULL_HANDLE;
  }
}

/*
*/
void
VulkanSwapChain::createImageViews()
{
  m_imageViews.resize(m_imageCount);
  for (uint32 i = 0; i < m_imageCount; i++) {
    VkImageViewCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = m_images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = m_colorFormat,
      .components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY
      },
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };
    VK_CHECK(vkCreateImageView(m_device, &createInfo, nullptr, &m_imageViews[i]));
  }
}

/*
*/
VkSemaphore
VulkanSwapChain::createSemaphore(const ANSICHAR* name) const
{
  const VkSemaphoreCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                                         .pNext = nullptr,
                                         .flags = 0};
  VkSemaphore semaphore = VK_NULL_HANDLE;
  VK_CHECK(vkCreateSemaphore(m_device, &createInfo, nullptr, &semaphore));
  g_vulkanAPI().setDebugName(VK_OBJECT_TYPE_SEMAPHORE, semaphore, name);
  return semaphore;
}
} // namespace chEngineSDK
