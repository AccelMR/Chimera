/************************************************************************/
/**
 * @file chRenderPass.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for the render pass. This is the base class for all render
 * passes. It is used to create render passes and allocate them.
 * It is also used to reset render passes and free them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
class CH_CORE_EXPORT IRenderPass {
 public:
  virtual ~IRenderPass() = default;
};

} // namespace chEngineSDK