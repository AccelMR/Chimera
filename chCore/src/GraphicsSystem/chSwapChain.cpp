/************************************************************************/
/**
 * @file chSwapChain.cpp
 * @author AccelMR
 * @date 2022/11/17
 *  Chimera swap chain interface that handles connection to a lower actual swap chain.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chSwapChain.h"

#include "chFence.h"

namespace chEngineSDK{

/*
*/
SwapChain::SwapChain() {}

/*
*/
void
SwapChain::init(const chGPUDesc::SwapChainDesc& desc) {
  _internalInit(desc);
}

/*
*/
void
SwapChain::cleanup() {
  _internalCleanup();
}

/*
*/
void
SwapChain::resize(uint32 width, uint32 height) {
  _internalResize(width, height);
}

/*
*/
uint32
SwapChain::getCurrentFrameIndex() const {
  return _internalGetCurrentFrameIndex();
}

/*
*/
bool
SwapChain::acquireNextFrame() {
  return _internalAcquireNextFrame();
}

/*
*/
SPtr<Texture>
SwapChain::getCurrentFrame() const {
  return _internalGetCurrentFrame();
}

/*
*/
FORMAT
SwapChain::getFormat() const {
  return _internalGetFormat();
}

/*
*/
void
SwapChain::setVSyncEnabled(bool enabled) {
  _internalSetVSyncEnabled(enabled);
}

/*
*/
void
SwapChain::waitForGPU() {
  _internalWaitForGPU();
}
}


