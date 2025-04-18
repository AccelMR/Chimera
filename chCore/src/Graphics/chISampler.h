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
class CH_CORE_EXPORT ISampler {
 public:
  virtual ~ISampler() = default;
};
} // namespace chEngineSDK