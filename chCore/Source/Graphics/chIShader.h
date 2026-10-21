/************************************************************************/
/**
 * @file chIShader.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Shader interface. This is the base class for all shaders.
 * It is used to create shaders and allocate them.
 * It is also used to reset shaders and free them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
class UUID;

class IShader {
 public:
  IShader() = default;
  virtual ~IShader() = default;

  virtual UUID
  getShaderId() const = 0;
};

} // namespace chEngineSDK
