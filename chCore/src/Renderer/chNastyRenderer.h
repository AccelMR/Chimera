/************************************************************************/
/**
 * @file chNastyRenderer.h
 * @author AccelMR
 * @date 2025/07/14
 * @brief Temporary forward renderer, replaced by the render graph.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"
#include "chIRenderer.h"
#include "chPipelineCache.h"

namespace chEngineSDK {

/**
 * Exists only until the render graph: one forward pass of the scene's render items into
 * the color target, with a depth target of its own and one texture per item.
 */
class CH_CORE_EXPORT NastyRenderer : public IRenderer
{
 public:
  NastyRenderer() = default;

  ~NastyRenderer() override;

  void
  initialize(uint32 width, uint32 height, Format colorFormat) override;

  void
  onRender(ICommandList& commandList,
           const ITextureView& colorTarget,
           const Scene& scene,
           const Camera& camera) override;

  void
  resize(uint32 width, uint32 height) override;

  void
  cleanup() override;

  NODISCARD uint32
  getWidth() const override { return m_renderWidth; }

  NODISCARD uint32
  getHeight() const override { return m_renderHeight; }

  void
  setClearColors(const Vector<LinearColor>& clearColors) override
  { m_clearColors = clearColors; }

 private:
  void
  createDepthTarget();

  void
  initializeRenderResources();

  Format m_colorFormat = Format::Unknown;
  uint32 m_renderWidth = 0;
  uint32 m_renderHeight = 0;
  Vector<LinearColor> m_clearColors;

  SPtr<ITexture> m_depthTarget;
  SPtr<ITextureView> m_depthTargetView;

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  PipelineCache m_pipelineCache;
  // Owned by m_pipelineCache; the targets never change format, so it is looked up once.
  const IPipeline* m_pipeline = nullptr;
  // Camera matrices read by the shader through the bindless heap. One per frame in flight,
  // because the CPU writes them while the GPU may still read the previous frame's.
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_cameraBuffers;

  // 1x1 white, for items without a texture.
  SPtr<ITexture> m_defaultTexture;
  SPtr<ISampler> m_sampler;

  // Kept between frames, so gathering the visible items does not allocate.
  Vector<const RenderItem*> m_visibleItems;
};
} // namespace chEngineSDK
