/************************************************************************/
/**
 * @file chPipelineLayout.h
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 * Pipeline layout class. This class is used to manage the pipeline layout
 * 
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class CH_CORE_EXPORT IPipelineLayout {
 public:
  virtual ~IPipelineLayout() = default;
};
} // namespace chEngineSDK