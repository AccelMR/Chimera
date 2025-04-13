/************************************************************************/
/**
 * @file chIFrameBuffer.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for the frame buffer. This is the base class for all frame
 * buffers. It is used to create frame buffers and allocate them.
 * It is also used to reset frame buffers and free them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class CH_CORE_EXPORT IFrameBuffer {
 public:
  virtual ~IFrameBuffer() = default;
  
  NODISCARD virtual uint32 
  getWidth() const = 0;
  
  NODISCARD virtual uint32 
  getHeight() const = 0;

  NODISCARD virtual uint32
  getLayers() const = 0;

  NODISCARD virtual SPtr<IRenderPass>
  getRenderPass() const = 0;

  NODISCARD virtual const Vector<SPtr<ITextureView>>&
  getAttachments() const = 0;
};
} // namespace chEngineSDK