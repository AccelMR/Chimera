/************************************************************************/
/**
 * @file chFence.h
 * @author AccelMR
 * @date 2025/01/27
 * 
 *  Fence GPU interface
 *
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
  * Includes
  */
/************************************************************************/
#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class CH_CORE_EXPORT Fence
{
 public:
  Fence() = default;
  virtual ~Fence() = default;

  void
  wait(uint64 timeout = UINT64_MAX);

  void
  reset();

  bool
  isSignaled() const;

 private: 
  virtual void
  _internalWait(uint64 timeout = UINT64_MAX) = 0;

  virtual void
  _internalReset() = 0;

  virtual bool
  _internalIsSignaled() const = 0;
};
} // namespace chEngineSDK