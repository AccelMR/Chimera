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
SwapChain::resize(uint32 frameCount, uint32 width, uint32 height, FORMAT fromat) {
  _internalResize(frameCount, width, height, fromat);
}

/*
*/
void
SwapChain::present(uint32 syncInterval, uint32 flags) {
  _internalPresent(syncInterval, flags);
}
}


