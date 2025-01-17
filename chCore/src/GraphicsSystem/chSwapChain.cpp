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

namespace chEngineSDK{

/*
*/
SwapChain::SwapChain()
  : m_frameCount(2),
    m_width(4),
    m_height(4),
    m_format(FORMAT::kR8G8B8A8_UNORM)
{}

/*
*/
void
SwapChain::resize(uint32 frameCount, uint32 width, uint32 height, FORMAT fromat) {
  _internalResize(frameCount, width, height, fromat);
}
}


