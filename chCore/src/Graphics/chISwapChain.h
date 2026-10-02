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
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
/**
 * Result of acquiring or presenting a swap chain image, so the caller knows whether it can
 * draw and whether the swap chain has to be recreated.
 */
enum class SwapChainStatus
{
  Ready,
  Suboptimal, // The image was acquired or presented, but the swap chain should be recreated.
  OutOfDate,  // No image was acquired; recreate the swap chain before drawing again.
  Failed      // No image was acquired; skip the frame.
};

class ISwapChain {
 public:
  virtual ~ISwapChain() = default;

  NODISCARD virtual SwapChainStatus
  acquireNextImage(SPtr<ISemaphore> waitSemaphore,
                   SPtr<IFence> fence = nullptr) = 0;

  NODISCARD virtual SwapChainStatus
  present(const Vector<SPtr<ISemaphore>>& waitSemaphores) = 0;

  virtual void
  resize(uint32 width, uint32 height) = 0;

  NODISCARD virtual uint32
  getCurrentImageIndex() const = 0;

  NODISCARD virtual SPtr<ITexture>
  getTexture(uint32 index) const = 0;

  NODISCARD virtual SPtr<ITextureView>
  getTextureView(uint32 index) const = 0;

  NODISCARD virtual SPtr<IRenderPass>
  getRenderPass() const = 0;

  NODISCARD virtual SPtr<IFrameBuffer>
  getFramebuffer(uint32 index) const = 0;

  NODISCARD virtual uint32
  getTextureCount() const = 0;

  NODISCARD virtual Format
  getFormat() const = 0;

  NODISCARD virtual uint32
  getWidth() const = 0;

  NODISCARD virtual uint32
  getHeight() const = 0;
};

} // namespace chEngineSDK
