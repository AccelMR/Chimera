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

#include "chGraphicsTypes.h"

namespace chEngineSDK {
/**
 * Draws the scene into a color target it does not own (the swap chain image), so the
 * application decides where the frame ends up and what is drawn over it.
 */
class CH_CORE_EXPORT IRenderer
{
 public:
  virtual ~IRenderer() = default;

  /**
   * The pipelines are built for colorFormat, the format of every target given to onRender.
   */
  virtual void
  initialize(uint32 width, uint32 height, Format colorFormat) = 0;

  /**
   * Records into the command list of the frame what the camera sees of the scene. The color
   * target must be in the RenderTarget state, has the size of the renderer, and is cleared.
   * Scene::updateTransforms must have run this frame.
   */
  virtual void
  onRender(ICommandList& commandList,
           const ITextureView& colorTarget,
           const Scene& scene,
           const Camera& camera) = 0;

  virtual void
  resize(uint32 width, uint32 height) = 0;

  virtual void
  cleanup() = 0;

  NODISCARD virtual uint32
  getWidth() const = 0;

  NODISCARD virtual uint32
  getHeight() const = 0;

  virtual void
  setClearColors(const Vector<LinearColor>& clearColors) = 0;
};
} // namespace chEngineSDK
