/************************************************************************/
/**
 * @file chGPUFramebuffer.cpp
 * @author AccelMR
 * @date 2025/02/01
 *   GPU Framebuffer abstraction for the engine.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chGPUFramebuffer.h"

namespace chEngineSDK {
/*
*/
void
Framebuffer::bind() {
  _internalBind();
}

/*
*/
uint32
Framebuffer::getWidth() const {
  return _internalGetWidth();
}

/*
*/
uint32
Framebuffer::getHeight() const {
  return _internalGetHeight();
}

/*
*/
const Vector<SPtr<Texture>>&
Framebuffer::getAttachments() const {
  return m_attachments;
}
} // namespace chEngineSDK