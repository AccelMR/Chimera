/************************************************************************/
/**
 * @file chSceneRenderer.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Builds and runs the render graph of each frame.
 */
/************************************************************************/

#include "chSceneRenderer.h"

#include "chAlgorithm.h"
#include "chCamera.h"
#include "chDebugLinePass.h"
#include "chDebugRenderPath.h"
#include "chDeferredRenderPath.h"
#include "chForwardRenderPath.h"
#include "chImageBasedLighting.h"
#include "chLightItem.h"
#include "chLogger.h"
#include "chProfiler.h"
#include "chShadowRenderer.h"
#include "chSkinningPass.h"
#include "chSkyPass.h"
#include "chStringUtils.h"
#include "chTonemapPass.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(SceneRendererLog, All);

namespace {
// Half floats keep light brighter than 1 and enough precision in the darks for the tonemap.
constexpr Format kSceneColorFormat = Format::R16G16B16A16_SFLOAT;
} // namespace

/*
 */
SceneRenderer::SceneRenderer(const SceneRendererDesc& desc)
  : m_desc(desc),
    m_forwardRenderPath(new ForwardRenderPath()),
    m_deferredRenderPath(new DeferredRenderPath()),
    m_renderPath(m_deferredRenderPath.get()),
    m_debugRenderPath(new DebugRenderPath()),
#if USING(CH_DEBUG_DRAW)
    m_debugLinePass(new DebugLinePass()),
#endif // USING(CH_DEBUG_DRAW)
    m_shadowRenderer(new ShadowRenderer()),
    m_skinningPass(new SkinningPass()),
    m_imageBasedLighting(new ImageBasedLighting()),
    m_skyPass(new SkyPass()),
    m_tonemapPass(new TonemapPass())
{}

/*
 */
SceneRenderer::~SceneRenderer() = default;

/*
 */
void
SceneRenderer::extract(const Scene& scene,
                       const Camera& camera,
                       const LinearColor& clearColor,
                       const DebugDraw* extraLines)
{
  CH_PROFILE_SCOPE("ExtractRenderFrame");
  m_frame.extract(scene, camera, clearColor, extraLines);
}

/*
 */
RenderGraph&
SceneRenderer::beginFrame()
{
  updateSettings();
  m_graph.reset();
  if (m_desc.bProfileGpu) {
    m_gpuProfiler.beginFrame();
  }
  return m_graph;
}

/*
 */
RGTextureHandle
SceneRenderer::addScenePasses(RGTextureHandle output)
{
  CH_PROFILE_SCOPE("AddScenePasses");
  // First, so every pass after it draws the skinned vertices.
  m_skinningPass->addPass(m_graph, m_frame.getWritableItems());
  const Camera& camera = m_frame.getCamera();
  const Frustum& frustum = camera.getFrustum();
  {
    CH_PROFILE_SCOPE("Gather");
    m_frame.gatherItems(frustum, m_visibleItems);
    m_frame.gatherLights(frustum, m_visibleLights);
    filterLightTypes();
  }
  const RenderView view{.camera = camera,
                        .items = m_visibleItems,
                        .lights = m_visibleLights,
                        .clearColor = m_frame.getClearColor(),
                        .bShowShadowCascades = m_viewMode == ViewMode::ShadowCascades,
                        .bScreenSpaceAO = m_bScreenSpaceAO};

  const RGTextureHandle depth = addViewModePasses(view, output);
#if USING(CH_DEBUG_DRAW)
  // The shadow map view shows textures, not the scene, so lines would not match it.
  if (g_cvarDebugLines.get() && m_viewMode != ViewMode::ShadowMaps) {
    m_debugLinePass->addPass(m_graph, camera, m_frame.getDebugLines(), output, depth);
  }
#endif // USING(CH_DEBUG_DRAW)
  return depth;
}

/*
 */
RGTextureHandle
SceneRenderer::addViewModePasses(const RenderView& view, RGTextureHandle output)
{
  switch (m_viewMode) {
    case ViewMode::Lit:
    case ViewMode::ShadowCascades:
      return addLitPasses(view, output);
    case ViewMode::LitWireframe: {
      const RGTextureHandle depth = addLitPasses(view, output);
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
    case ViewMode::ScreenSpaceAO:
    case ViewMode::Emissive:
      return m_deferredRenderPath->addGBufferViewPasses(m_graph, view, output, m_viewMode);
    case ViewMode::ShadowMaps:
      // The views are only needed by lit paths; the debug pass reads the atlases.
      static_cast<void>(m_shadowRenderer->addPasses(m_graph, m_frame, view));
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
  if (m_desc.bProfileGpu) {
    const uint32 gpuScope = m_gpuProfiler.beginScope(commandList, "RenderGraph");
    m_graph.execute(commandList, m_texturePool, &m_gpuProfiler);
    m_gpuProfiler.endScope(commandList, gpuScope);
  }
  else {
    m_graph.execute(commandList, m_texturePool);
  }
  m_frameCommandStats = commandList.getStats() - statsBefore;
#else
  m_graph.execute(commandList, m_texturePool);
#endif // USING(CH_PROFILER)
  m_texturePool.nextFrame();
}

/*
 */
RGTextureHandle
SceneRenderer::addLitPasses(const RenderView& view, RGTextureHandle output)
{
  const RenderView litView{.camera = view.camera,
                           .items = view.items,
                           .lights = view.lights,
                           .shadows = m_bShadows
                                          ? m_shadowRenderer->addPasses(m_graph, m_frame, view)
                                          : Span<const ShadowView>{},
                           .ibl = m_imageBasedLighting->addPasses(
                               m_graph, m_frame.getSkyLight()),
                           .clearColor = view.clearColor,
                           .bShowShadowCascades = view.bShowShadowCascades,
                           .bScreenSpaceAO = view.bScreenSpaceAO};
  const RGTextureDesc outputDesc = m_graph.getTextureDesc(output);
  const RGTextureHandle sceneColor = m_graph.createTexture(
      "SceneColor",
      {.format = kSceneColorFormat, .width = outputDesc.width, .height = outputDesc.height});
  const RGTextureHandle depth = m_renderPath->addPasses(m_graph, litView, sceneColor);
  m_skyPass->addPass(m_graph, view.camera, m_frame.getSkyLight(), sceneColor, depth);
  m_tonemapPass->addPass(m_graph, sceneColor, output);
  return depth;
}

/*
 */
void
SceneRenderer::filterLightTypes()
{
  const bool bDirectional = g_cvarDirectionalLights.get();
  const bool bPoint = g_cvarPointLights.get();
  const bool bSpot = g_cvarSpotLights.get();
  if (bDirectional && bPoint && bSpot) {
    return;
  }

  const auto isOff = [bDirectional, bPoint, bSpot](const LightItem* light) {
    switch (light->type) {
      case LightType::Directional:
        return !bDirectional;
      case LightType::Point:
        return !bPoint;
      case LightType::Spot:
        return !bSpot;
    }
    return false;
  };
  Algorithm::removeAllIf(m_visibleLights, isOff);
}

/*
 */
void
SceneRenderer::updateSettings()
{
  const String& disabledPassesText = g_cvarDisabledPasses.get();
  if (disabledPassesText != m_disabledPassesText) {
    m_disabledPassesText = disabledPassesText;
    m_disabledPasses.clear();
    for (const String& name : StringUtils::splitString(disabledPassesText, ',')) {
      const StringView trimmed = StringUtils::trimView(name);
      if (!trimmed.empty()) {
        m_disabledPasses.emplace_back(trimmed);
      }
    }
    m_graph.setDisabledPasses(m_disabledPasses);
  }

  const String& viewModeText = g_cvarViewMode.get();
  if (m_desc.viewMode) {
    m_viewMode = *m_desc.viewMode;
  }
  else if (viewModeText != m_viewModeText) {
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
