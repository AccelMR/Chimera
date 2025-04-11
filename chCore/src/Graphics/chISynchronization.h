/************************************************************************/
/**
 * @file chISynchronization.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for synchronization primitives. This is the base class for all
 * synchronization primitives. It is used to synchronize the CPU and GPU.
 * It is also used to synchronize command buffers and fences.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

class CH_CORE_EXPORT ISemaphore {
 public:
  ISemaphore() = default;
  virtual ~ISemaphore() = default;
};

class CH_CORE_EXPORT IFence {
 public:
  IFence() = default;
  virtual ~IFence() = default;

  virtual void 
  wait(uint64 timeout = UINT64_MAX) = 0;

  virtual void 
  reset() = 0;

  NODISCARD virtual bool 
  isSignaled() const = 0;
};

} // namespace chEngineSDK