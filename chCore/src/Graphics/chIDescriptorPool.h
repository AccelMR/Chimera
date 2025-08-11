/************************************************************************/
/**
 * @file chDescriptorPool.h
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 * Descriptor pool class. This class is used to manage descriptor sets
 * and allocate them from the pool.
 * It is used to create descriptor sets and allocate them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
class IDescriptorPool {
 public:
  virtual ~IDescriptorPool() = default;
  /**
   * @brief Get the raw handle of the descriptor pool.
   * @return The raw handle of the descriptor pool.
   */
  virtual void*
  getRaw() const = 0;

  /**
   * @brief Allocate a descriptor set from the pool.
   * @param allocInfo Information about the descriptor set to allocate.
   * @return A shared pointer to the allocated descriptor set.
   */
  virtual SPtr<IDescriptorSet>
  allocateDescriptorSet(const DescriptorSetAllocateInfo& allocInfo) = 0;
};
} // namespace chEngineSDK
