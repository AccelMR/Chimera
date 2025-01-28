/************************************************************************/
/**
 * @file chSwapChain.h
 * @author AccelMR
 * @date 2022/11/17
 *  Chimera swap chain interface that handles connection to a lower actual swap chain.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chPrerequisitesGraphics.h"
#include "chFormats.h"

namespace chEngineSDK {
class CH_CORE_EXPORT SwapChain {
 public:
 /*
  *   Default constructor
  */
  SwapChain();

 /*
  *   Default destructor
  */
  virtual ~SwapChain() = default;

  void
  resize(uint32 width, uint32 height);

  void
  present(uint32 syncInterval, uint32 flags);

  uint32
  getCurrentFrameIndex() const;

  SPtr<Fence>
  getCurrentFrameFence() const;

 protected:
  virtual void
  _internalResize(uint32 width, uint32 height) = 0;

  virtual void
  _internalPresent(uint32 syncInterval, uint32 flags) = 0;

  virtual uint32
  _internalGetCurrentFrameIndex() const = 0;

  virtual SPtr<Fence>
  _internalGetCurrentFrameFence() const = 0;

};
}

