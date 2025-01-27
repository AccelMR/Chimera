/************************************************************************/
/**
 * @file chVulkanSampler.h
 * @author AccelMR
 * @date 2025/01/26
 *   Vulkan implementation of a GPU sampler.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chSampler.h"

#include "chVulkanGraphicsModule.h"

namespace chEngineSDK {
/**
 * @class VulkanSampler
 * @brief Vulkan implementation of a GPU sampler.
 */
class VulkanSampler final : public Sampler {
 public:
  /**
   * @brief Default constructor
   */
  VulkanSampler() = default;

  /**
   * @brief Default destructor
   */
  ~VulkanSampler() = default;

  /**
   * @brief Initializes the sampler.
   */
  void
  _internalInit(const chGPUDesc::SamplerDesc& desc) override;

  FORCEINLINE const VkSampler&
  getSampler() const {
    return m_sampler;
  }

 private:
  VkSampler m_sampler = VK_NULL_HANDLE;
};
} // namespace chEngineSDK
