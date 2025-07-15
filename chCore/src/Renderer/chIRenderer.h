/************************************************************************/
/**
 * @file chIRenderer.h
 * @author AccelMR
 * @date 2025/07/14
 * @brief  Interface for the Renderer module in the Chimera Engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
/**
 * @brief Renderer output containing the rendered result
 */
struct RendererOutput {
  SPtr<ITextureView> colorTarget;
  SPtr<ITextureView> depthTarget;
  uint32 width;
  uint32 height;
  bool isValid = false;
};

/**
 * @brief Base renderer interface for deferred rendering
 */
class CH_CORE_EXPORT IRenderer
{
 public:
  virtual ~IRenderer() = default;

  /**
   * @brief Initialize the renderer with target dimensions
   * @param width Target render width
   * @param height Target render height
   */
  virtual void
  initialize(uint32 width, uint32 height) = 0;

  /**
   * @brief Render to internal targets
   * @param deltaTime Time since last frame
   * @return RendererOutput containing the rendered result
   */
  virtual RendererOutput
  onRender(float deltaTime) = 0;

  /**
   * @brief Resize renderer targets
   * @param width New width
   * @param height New height
   */
  virtual void
  resize(uint32 width, uint32 height) = 0;

  /**
   * @brief Cleanup renderer resources
   */
  virtual void
  cleanup() = 0;

  /**
   * @brief Get current render target dimensions
   */
  virtual uint32
  getWidth() const = 0;

  virtual uint32
  getHeight() const = 0;
};
} // namespace chEngineSDK
