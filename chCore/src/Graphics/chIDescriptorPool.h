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
class CH_CORE_EXPORT IDescriptorPool {
 public:
  virtual ~IDescriptorPool() = default;

  virtual SPtr<IDescriptorSet>
  allocateDescriptorSet(const DescriptorSetAllocateInfo& allocInfo) = 0;
};
} // namespace chEngineSDK