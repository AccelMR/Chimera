/************************************************************************/
/**
 * @file chDebugRenderPath.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Draws scene data (unlit texture, normals, depth, UVs, edges) instead of the lit scene.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIRenderPath.h"
#include "chPipelineCache.h"
#include "chRenderSettings.h"

namespace chEngineSDK {

/**
 * Exists to look at what the scene feeds the renderer: one shader shows the unlit texture,
 * world normals, depth or texture coordinates, and draws the edges of the items as lines,
 * alone or over what another path drew.
 */
class CH_CORE_EXPORT DebugRenderPath : public IRenderPath
{
 public:
  /**
   * Needs a started IGraphicsAPI.
   */
  DebugRenderPath();

  ~DebugRenderPath() override;

  /**
   * Any mode but Lit and LitWireframe, which need a lit path.
   */
  void
  setViewMode(ViewMode mode);

  RGTextureHandle
  addPasses(RenderGraph& graph, const RenderView& view, RGTextureHandle output) override;

  /**
   * Draws the edges of the items over output, hidden where depth (the scene depth another
   * path returned) is in front of them.
   */
  void
  addWireframeOverlay(RenderGraph& graph,
                      const RenderView& view,
                      RGTextureHandle output,
                      RGTextureHandle depth);

 private:
  enum class PipelineKind : uint32
  {
    Solid,
    Lines,
    LinesOverDepth,
    COUNT
  };

  struct PipelineEntry
  {
    const IPipeline* pipeline = nullptr;
    Format colorFormat = Format::Unknown;
    Format depthFormat = Format::Unknown;
  };

  struct DrawSettings
  {
    const Camera* camera;
    Span<const RenderItem* const> items;
    const IPipeline* pipeline;
    uint32 shaderMode;
    // Four floats, used by the line modes.
    const float* color;
  };

  void
  recordDraws(RenderPassContext& context, const DrawSettings& settings);

  NODISCARD const IPipeline&
  getPipeline(PipelineKind kind, Format colorFormat, Format depthFormat);

  ViewMode m_viewMode = ViewMode::Unlit;

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  PipelineCache m_pipelineCache;
  // Looked up again only when a target format changes, because building the description
  // allocates (VertexLayout).
  Array<PipelineEntry, static_cast<SIZE_T>(PipelineKind::COUNT)> m_pipelines{};

  // One per frame in flight, because the CPU writes them while the GPU may still read the
  // previous frame's.
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_cameraBuffers;

  // 1x1 white, for items without a texture.
  SPtr<ITexture> m_defaultTexture;
  SPtr<ISampler> m_sampler;
};

} // namespace chEngineSDK
