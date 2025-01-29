/************************************************************************/
/**
 * @file chVulkanFence.h
 * @author AccelMR
 * @date 2025/01/27
 * 
 *  Vulkan Fence implementation.
 *
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include "chFence.h"

namespace chEngineSDK {
class VulkanFence final : public Fence
{
 public:
  VulkanFence() = delete;
  VulkanFence(VkDevice device);
  ~VulkanFence();

  void
  _internalWait(uint64 timeout = UINT64_MAX) override;

  void
  _internalReset() override;

  bool
  _internalIsSignaled() const override;

  FORCEINLINE VkFence
  getFence() const {
    return m_fence;
  }

 private:
  VkFence m_fence;
  VkDevice m_device;
};
} // namespace chEngineSDK