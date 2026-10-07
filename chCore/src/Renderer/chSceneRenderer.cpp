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

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(SceneRendererLog, All);

/*
 */
SceneRenderer::SceneRenderer()
  : m_renderPath(new ForwardRenderPath()),
    m_debugRenderPath(new DebugRenderPath())
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
      return m_renderPath->addPasses(m_graph, view, output);
    case ViewMode::LitWireframe: {
      const RGTextureHandle depth = m_renderPath->addPasses(m_graph, view, output);
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
