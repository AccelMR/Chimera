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
class CH_CORE_EXPORT ISwapChain {
 public:
  virtual ~ISwapChain() = default;

  virtual bool
  acquireNextImage(SPtr<ISemaphore> waitSemaphore,
                   SPtr<IFence> fence = nullptr) = 0;

  virtual void
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
