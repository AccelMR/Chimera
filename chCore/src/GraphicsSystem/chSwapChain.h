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
  resize(uint32 frameCount, uint32 width, uint32 height, FORMAT fromat);

  void
  moveNextFrame();

  /** 
   *   Returns frame count
   **/
  FORCEINLINE uint32
  getFrameCounter() const;

  /**
   *   Returns the height of the swapchain
   **/
  FORCEINLINE uint32
  getHeight() const;

  /**
   *   Returns the width of the swapchain
   **/
  FORCEINLINE uint32
  getWidth() const;
  
  /**
   *   Returns the width of the swapchain
   **/
  FORCEINLINE uint32
  getCurrentFrameIndex() const;

  /**
   *   Returns the current render target
   **/
  virtual SPtr<Texture>
  getCurrentRenderTarget() const = 0; //TODO not  pure

protected:
  friend class GraphicsModule;

  virtual void
  init(const chGPUDesc::SwapChainDesc& desc) = 0;

  virtual void
  _internalResize(uint32 frameCount, uint32 width, uint32 height, FORMAT fromat) = 0;

  virtual uint32
  _internalGetCurrentFrameIndex() const = 0;

protected:
  uint32 m_frameCount;
  uint32 m_width;
  uint32 m_height;
  FORMAT m_format;
  //Vector<SPtr<Texture>> m_renderTargets;
 
};


/************************************************************************/
/*
 * Implementations
 */                                                                     
/***********************************************************************
/*
*/
FORCEINLINE uint32 
SwapChain::getFrameCounter() const {
  return m_frameCount;
}

/*
*/
FORCEINLINE uint32 
SwapChain::getCurrentFrameIndex() const {
  return _internalGetCurrentFrameIndex();
}

/*
*/
FORCEINLINE uint32 
SwapChain::getHeight() const {
  return m_height;
}

/*
*/
FORCEINLINE uint32 
SwapChain::getWidth() const {
  return m_width;
}

}

