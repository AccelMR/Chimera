/************************************************************************/
/**
 * @file chIPipeline.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for the pipeline. This is the base class for all pipelines.
 * It is used to create pipelines and allocate them.
 * It is also used to reset pipelines and free them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
class IPipeline {
 public:
  virtual ~IPipeline() = default;
};
} // namespace chEngineSDK