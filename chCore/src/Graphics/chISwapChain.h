/************************************************************************/
/**
 * @file chISwapChain.h
 * @author AccelMR
 * @date 2025/04/07
 * @details
 * Interface for the swap chain, the images a window shows.
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

/**
 * Images of one window. A frame acquires an image, draws it from Undefined to RenderTarget
 * and leaves it in Present; the synchronization with the frame stays inside the graphics API.
 */
class ISwapChain
{
 public:
  virtual ~ISwapChain() = default;

  /**
   * Called between IGraphicsAPI::beginFrame and endFrame; the frame waits for the image
   * before it draws.
   */
  NODISCARD virtual SwapChainStatus
  acquireNextImage() = 0;

  /**
   * Called after IGraphicsAPI::endFrame, for an image acquired in that frame.
   */
  NODISCARD virtual SwapChainStatus
  present() = 0;

  virtual void
  resize(uint32 width, uint32 height) = 0;

  NODISCARD virtual const ITexture&
  getCurrentTexture() const = 0;

  NODISCARD virtual const ITextureView&
  getCurrentTextureView() const = 0;

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
