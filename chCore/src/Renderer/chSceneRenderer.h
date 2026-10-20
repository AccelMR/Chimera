/************************************************************************/
/**
 * @file chSceneRenderer.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Builds and runs the render graph of each frame.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGpuProfiler.h"
#include "chIRenderPath.h"
#include "chRenderFrame.h"
#include "chRenderGraph.h"
#include "chRenderSettings.h"
#include "chTransientTexturePool.h"

namespace chEngineSDK {

class DebugLinePass;
class DebugRenderPath;
class DeferredRenderPath;
class ForwardRenderPath;
class ImageBasedLighting;
class ShadowRenderer;
class SkyPass;
class TonemapPass;

/**
 * What one frame of a SceneRenderer drew, for the profiler.
 */
struct RenderFrameStats
{
  Span<const RenderPassStats> passes;
  // Every pass plus the barriers at the end of the frame.
  CommandListStats commands;
  uint32 visibleItems = 0;
  uint32 visibleLights = 0;
};

/**
 * Exists so an application only says where the frame goes and what it draws on top: it
 * owns the frame's render graph, the textures the graph makes, and the render path that
 * draws a scene. Lit view modes draw the shadow maps first, then the scene (with the sky
 * light's image behind it) into an HDR scene color that a tonemap pass takes to the output;
 * the debug views write the output directly. A frame is beginFrame(), imports and
 * addScenePasses() (plus any passes of the application, the editor UI), then execute().
 * The scene passes draw what extract() copied at the end of the update, never the Scene,
 * so the scene can change while the frame is built.
 * What the scene passes show follows the Renderer.ViewMode console variable, and the path
 * of the Lit modes follows Renderer.Path, both read once per frame.
 *
 * Needs a started IGraphicsAPI and must be destroyed before it shuts down.
 */
class CH_CORE_EXPORT SceneRenderer
{
 public:
  SceneRenderer();

  ~SceneRenderer();

  SceneRenderer(const SceneRenderer&) = delete;

  SceneRenderer&
  operator=(const SceneRenderer&) = delete;

  /**
   * Copies what the scene passes of the next frame draw: the scene as it is now, seen by
   * camera, plus the application's lines (extraLines, may be null). Call it at the end of
   * the update, after Scene::updateTransforms.
   */
  void
  extract(const Scene& scene,
          const Camera& camera,
          const LinearColor& clearColor,
          const DebugDraw* extraLines = nullptr);

  /**
   * What the last extract() copied.
   */
  NODISCARD FORCEINLINE const RenderFrame&
  getFrame() const
  {
    return m_frame;
  }

  /**
   * Forgets the last frame's graph and returns it empty.
   */
  NODISCARD RenderGraph&
  beginFrame();

  /**
   * Adds the passes that draw what the camera of the extracted frame sees into output, then
   * the frame's debug lines.
   *
   * @return the scene depth, for passes the application adds after; invalid in view modes
   *         without one.
   */
  RGTextureHandle
  addScenePasses(RGTextureHandle output);

  NODISCARD FORCEINLINE ViewMode
  getViewMode() const
  {
    return m_viewMode;
  }

  /**
   * Compiles the graph and records it into the command list of the frame.
   */
  void
  execute(ICommandList& commandList);

  /**
   * What the last frame drew. Kept until the next execute().
   */
  NODISCARD RenderFrameStats
  getLastFrameStats() const
  {
    return {.passes = m_graph.getExecutedPassStats(),
            .commands = m_frameCommandStats,
            .visibleItems = static_cast<uint32>(m_visibleItems.size()),
            .visibleLights = static_cast<uint32>(m_visibleLights.size())};
  }

 private:
  /**
   * What the view mode draws; returns the scene depth like addScenePasses.
   */
  RGTextureHandle
  addViewModePasses(const RenderView& view, RGTextureHandle output);

  /**
   * The shadow maps, the active render path into an HDR scene color, then the tonemap into
   * output.
   */
  RGTextureHandle
  addLitPasses(const RenderView& view, RGTextureHandle output);

  /**
   * Drops the visible lights of the types the Renderer.*Lights settings turn off.
   */
  void
  filterLightTypes();

  void
  updateSettings();

  // Written by extract() in the update, read while the next frame is built.
  RenderFrame m_frame;
  RenderGraph m_graph;
  TransientTexturePool m_texturePool;
  UniquePtr<ForwardRenderPath> m_forwardRenderPath;
  UniquePtr<DeferredRenderPath> m_deferredRenderPath;
  // The one of the two above that draws the Lit view modes.
  IRenderPath* m_renderPath = nullptr;
  UniquePtr<DebugRenderPath> m_debugRenderPath;
#if USING(CH_DEBUG_DRAW)
  UniquePtr<DebugLinePass> m_debugLinePass;
#endif // USING(CH_DEBUG_DRAW)
  UniquePtr<ShadowRenderer> m_shadowRenderer;
  UniquePtr<ImageBasedLighting> m_imageBasedLighting;
  UniquePtr<SkyPass> m_skyPass;
  UniquePtr<TonemapPass> m_tonemapPass;

  ViewMode m_viewMode = ViewMode::Lit;
  // The console variable texts the settings were parsed from, so they are parsed only on
  // change.
  String m_viewModeText;
  String m_renderPathText;
  String m_disabledPassesText;
  // Parsed from Renderer.DisabledPasses; the graph keeps views of them.
  Vector<String> m_disabledPasses;

  // Kept between frames, so gathering the visible items and lights does not allocate.
  Vector<const RenderItem*> m_visibleItems;
  Vector<const LightItem*> m_visibleLights;
  CommandListStats m_frameCommandStats;
  GpuProfiler m_gpuProfiler;
};

} // namespace chEngineSDK
