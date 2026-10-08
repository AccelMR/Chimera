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
#include "chForwardRenderPath.h"
#include "chLogger.h"
#include "chScene.h"
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
  : m_renderPath(new ForwardRenderPath()),
    m_debugRenderPath(new DebugRenderPath()),
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
  updateViewMode();
  m_graph.reset();
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
  scene.gatherRenderItems(camera.getFrustum(), m_visibleItems);
  const RenderView view{.camera = camera, .items = m_visibleItems, .clearColor = clearColor};

  switch (m_viewMode) {
    case ViewMode::Lit:
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
  m_graph.compile();
  m_graph.execute(commandList, m_texturePool);
  m_texturePool.nextFrame();
}

/*
 */
RGTextureHandle
SceneRenderer::addLitPasses(const RenderView& view, RGTextureHandle output)
{
  const RGTextureDesc outputDesc = m_graph.getTextureDesc(output);
  const RGTextureHandle sceneColor = m_graph.createTexture(
      "SceneColor",
      {.format = kSceneColorFormat, .width = outputDesc.width, .height = outputDesc.height});
  const RGTextureHandle depth = m_renderPath->addPasses(m_graph, view, sceneColor);
  m_tonemapPass->addPass(m_graph, sceneColor, output);
  return depth;
}

/*
 */
void
SceneRenderer::updateViewMode()
{
  const String& text = g_cvarViewMode.get();
  if (text == m_viewModeText) {
    return;
  }
  m_viewModeText = text;

  const Optional<ViewMode> mode = ViewModeUtils::fromName(text);
  if (!mode) {
    CH_LOG_WARNING(SceneRendererLog, "Unknown view mode '{0}', using Lit.", text);
  }
  m_viewMode = mode.value_or(ViewMode::Lit);
}

} // namespace chEngineSDK
