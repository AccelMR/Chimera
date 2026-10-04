/************************************************************************/
/**
 * @file ISampler.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 *  Sampler interface. This is the base class for all samplers.
 *  It is used to create samplers and allocate them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class ISampler {
 public:
  virtual ~ISampler() = default;

  /**
   * Index of this sampler in the bindless sampler heap (SamplerDescriptorHeap in HLSL).
   */
  NODISCARD virtual uint32
  getBindlessIndex() const = 0;
};
} // namespace chEngineSDK