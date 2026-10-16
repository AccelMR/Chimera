/************************************************************************/
/**
 * @file chSceneRenderer.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Builds and runs the render graph of each frame.
 */
/************************************************************************/

#include "chSceneRenderer.h"

#include "chCamera.h"
#include "chDebugRenderPath.h"
#include "chDeferredRenderPath.h"
#include "chForwardRenderPath.h"
#include "chLogger.h"
#include "chProfiler.h"
#include "chScene.h"
#include "chShadowRenderer.h"
#include "chTonemapPass.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(SceneRendererLog, All);

namespace {
// Half floats keep light brighter than 1 and enough precision in the darks for the tonemap.
constexpr Format kSceneColorFormat = Format::R16G16B16A16_SFLOAT;
} // namespace

/*
 */
SceneRenderer::SceneRenderer()
  : m_forwardRenderPath(new ForwardRenderPath()),
    m_deferredRenderPath(new DeferredRenderPath()),
    m_renderPath(m_deferredRenderPath.get()),
    m_debugRenderPath(new DebugRenderPath()),
    m_shadowRenderer(new ShadowRenderer()),
    m_tonemapPass(new TonemapPass())
{}

/*
 */
SceneRenderer::~SceneRenderer() = default;

/*
 */
RenderGraph&
SceneRenderer::beginFrame()
{
  updateSettings();
  m_graph.reset();
  m_gpuProfiler.beginFrame();
  return m_graph;
}

/*
 */
RGTextureHandle
SceneRenderer::addScenePasses(const Scene& scene,
                              const Camera& camera,
                              RGTextureHandle output,
                              const LinearColor& clearColor)
{
  CH_PROFILE_SCOPE("AddScenePasses");
  const Frustum& frustum = camera.getFrustum();
  {
    CH_PROFILE_SCOPE("Gather");
    scene.gatherRenderItems(frustum, m_visibleItems);
    scene.gatherLights(frustum, m_visibleLights);
  }
  const RenderView view{.camera = camera,
                        .items = m_visibleItems,
                        .lights = m_visibleLights,
                        .clearColor = clearColor,
                        .bShowShadowCascades = m_viewMode == ViewMode::ShadowCascades};

  switch (m_viewMode) {
    case ViewMode::Lit:
    case ViewMode::ShadowCascades:
      return addLitPasses(scene, view, output);
    case ViewMode::LitWireframe: {
      const RGTextureHandle depth = addLitPasses(scene, view, output);
      if (depth.isValid()) {
        m_debugRenderPath->addWireframeOverlay(m_graph, view, output, depth);
      }
      return depth;
    }
    case ViewMode::Wireframe:
    case ViewMode::Unlit:
    case ViewMode::Normals:
    case ViewMode::Depth:
    case ViewMode::TexCoords:
      m_debugRenderPath->setViewMode(m_viewMode);
      return m_debugRenderPath->addPasses(m_graph, view, output);
    case ViewMode::BaseColor:
    case ViewMode::Metallic:
    case ViewMode::Roughness:
    case ViewMode::AmbientOcclusion:
    case ViewMode::Emissive:
      return m_deferredRenderPath->addGBufferViewPasses(m_graph, view, output, m_viewMode);
    case ViewMode::ShadowMaps:
      // The views are only needed by lit paths; the debug pass reads the atlases.
      static_cast<void>(m_shadowRenderer->addPasses(m_graph, scene, view));
      m_shadowRenderer->addDebugPass(m_graph, output);
      return {};
    case ViewMode::COUNT:
      break;
  }
  return {};
}

/*
 */
void
SceneRenderer::execute(ICommandList& commandList)
{
  {
    CH_PROFILE_SCOPE("CompileGraph");
    m_graph.compile();
  }

  CH_PROFILE_SCOPE("ExecuteGraph");
#if USING(CH_PROFILER)
  const CommandListStats statsBefore = commandList.getStats();
  const uint32 gpuScope = m_gpuProfiler.beginScope(commandList, "RenderGraph");
  m_graph.execute(commandList, m_texturePool, &m_gpuProfiler);
  m_gpuProfiler.endScope(commandList, gpuScope);
  m_frameCommandStats = commandList.getStats() - statsBefore;
#else
  m_graph.execute(commandList, m_texturePool);
#endif // USING(CH_PROFILER)
  m_texturePool.nextFrame();
}

/*
 */
RGTextureHandle
SceneRenderer::addLitPasses(const Scene& scene, const RenderView& view, RGTextureHandle output)
{
  const RenderView litView{.camera = view.camera,
                           .items = view.items,
                           .lights = view.lights,
                           .shadows = m_shadowRenderer->addPasses(m_graph, scene, view),
                           .clearColor = view.clearColor,
                           .bShowShadowCascades = view.bShowShadowCascades};
  const RGTextureDesc outputDesc = m_graph.getTextureDesc(output);
  const RGTextureHandle sceneColor = m_graph.createTexture(
      "SceneColor",
      {.format = kSceneColorFormat, .width = outputDesc.width, .height = outputDesc.height});
  const RGTextureHandle depth = m_renderPath->addPasses(m_graph, litView, sceneColor);
  m_tonemapPass->addPass(m_graph, sceneColor, output);
  return depth;
}

/*
 */
void
SceneRenderer::updateSettings()
{
  const String& viewModeText = g_cvarViewMode.get();
  if (viewModeText != m_viewModeText) {
    m_viewModeText = viewModeText;
    const Optional<ViewMode> mode = RenderSettingsUtils::viewModeFromName(viewModeText);
    if (!mode) {
      CH_LOG_WARNING(SceneRendererLog, "Unknown view mode '{0}', using Lit.", viewModeText);
    }
    m_viewMode = mode.value_or(ViewMode::Lit);
  }

  const String& renderPathText = g_cvarRenderPath.get();
  if (renderPathText != m_renderPathText) {
    m_renderPathText = renderPathText;
    const Optional<RenderPathType> path =
        RenderSettingsUtils::renderPathFromName(renderPathText);
    if (!path) {
      CH_LOG_WARNING(SceneRendererLog, "Unknown render path '{0}', using Deferred.",
                     renderPathText);
    }
    if (path.value_or(RenderPathType::Deferred) == RenderPathType::Forward) {
      m_renderPath = m_forwardRenderPath.get();
    }
    else {
      m_renderPath = m_deferredRenderPath.get();
    }
  }
}

} // namespace chEngineSDK
