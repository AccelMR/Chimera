/************************************************************************/
/**
 * @file chDescriptorSet.h
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 * Descriptor set class. This class is used to manage descriptor sets
 * and allocate them from the pool.
 * It is used to create descriptor sets and allocate them.
 *
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class CH_CORE_EXPORT IDescriptorSet {
 public:
  virtual ~IDescriptorSet() = default;

  virtual void*
  getRaw() const = 0;
};
} // namespace chEngineSDK
