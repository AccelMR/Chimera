/************************************************************************/
/**
 * @file chICommandPool.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for the command pool. This is the base class for all command
 * pools. It is used to create command buffers and allocate them.
 * It is also used to reset command buffers and free them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class CH_CORE_EXPORT ICommandPool {
 public:
  ICommandPool() = default;
  virtual ~ICommandPool() = default;

  NODISCARD virtual SPtr<ICommandBuffer> 
  allocateCommandBuffer() = 0;

  virtual void 
  reset() = 0;
};
} // namespace chEngineSDK