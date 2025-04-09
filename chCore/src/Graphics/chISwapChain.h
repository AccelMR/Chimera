/************************************************************************/
/**
 * @file chISwapChain.h
 * @author AccelMR
 * @date 2025/04/07
 * @details 
 * SwapChain interface.
 * This interface is used to create and manage the swap chain.
 * It is used to create the swap chain, and to present the swap chain.
 * It is used by the graphics API to create the swap chain.
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"


namespace chEngineSDK {
class ISwapChain {
 public:
  virtual ~ISwapChain() = default;

  virtual void
  create(uint32 width, uint32 height, bool vsync = false) = 0;

  virtual void
  present() = 0;

  virtual void
  cleanUp() = 0;

  virtual void
  resize(uint32 width, uint32 height) = 0;

  virtual void*
  getNativeHandle() const = 0;
};

} // namespace chEngineSDK