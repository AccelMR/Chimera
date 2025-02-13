/************************************************************************/
/**
 * @file chGPUFramebuffer.h
 * @author AccelMR
 * @date 2025/02/01
 *   GPU Framebuffer abstraction for the engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chTexture.h"

namespace chEngineSDK {

class Framebuffer {
 public:
  virtual ~Framebuffer() = default;

  /** 
   * Binds the framebuffer for rendering.
   */
  void 
  bind();

  /**
   * Returns the width of the framebuffer.
   */
  uint32 
  getWidth() const;

  /**
   * Returns the height of the framebuffer.
   */
  uint32 
  getHeight() const;

  /**
   * Returns the attachments associated with this framebuffer.
   */
  const Vector<SPtr<Texture>>& 
  getAttachments() const;

 protected:

   /** 
   * Binds the framebuffer for rendering.
   */
  virtual void 
  _internalBind() = 0 ;

  /**
   * Returns the width of the framebuffer.
   */
  virtual uint32 
  _internalGetWidth() const = 0 ;

  /**
   * Returns the height of the framebuffer.
   */
  virtual uint32 
  _internalGetHeight() const = 0 ;

  /**
   * Returns the attachments associated with this framebuffer.
   */
  virtual const Vector<SPtr<Texture>>& 
  _internalGetAttachments() const = 0 ;

  
 protected:
  uint32 m_width;
  uint32 m_height;
  Vector<SPtr<Texture>> m_attachments;
};

} // namespace chEngineSDK
