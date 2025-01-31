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
  init(const chGPUDesc::SwapChainDesc& desc);

  void
  cleanup();

  void
  resize(uint32 width, uint32 height);

  uint32
  getCurrentFrameIndex() const;

  bool
  acquireNextFrame();

  SPtr<Texture>
  getCurrentFrame(const SPtr<GPUPipelineState>& pipeline) const;

  FORMAT
  getFormat() const;

  void
  setVSyncEnabled(bool enabled);

  void
  waitForGPU();

 protected:
   virtual void
  _internalInit(const chGPUDesc::SwapChainDesc& desc) = 0;

  virtual void
  _internalCleanup() = 0;

  virtual void
  _internalResize(uint32 width, uint32 height) = 0;

  virtual uint32
  _internalGetCurrentFrameIndex() const = 0;

  virtual bool
  _internalAcquireNextFrame() = 0;

  virtual SPtr<Texture>
  _internalGetCurrentFrame(const SPtr<GPUPipelineState>& pipeline) const = 0;

  virtual FORMAT
  _internalGetFormat() const = 0;

  virtual void
  _internalSetVSyncEnabled(bool enabled) = 0;

  virtual void
  _internalWaitForGPU() = 0;
};
}

