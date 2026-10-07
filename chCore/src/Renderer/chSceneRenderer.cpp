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
#include "chForwardRenderPath.h"
#include "chScene.h"

namespace chEngineSDK {

/*
 */
SceneRenderer::SceneRenderer()
  : m_renderPath(new ForwardRenderPath())
{}

/*
 */
SceneRenderer::~SceneRenderer() = default;

/*
 */
RenderGraph&
SceneRenderer::beginFrame()
{
  m_graph.reset();
  return m_graph;
}

/*
 */
void
SceneRenderer::addScenePasses(const Scene& scene,
                              const Camera& camera,
                              RGTextureHandle output,
                              const LinearColor& clearColor)
{
  scene.gatherRenderItems(camera.getFrustum(), m_visibleItems);
  m_renderPath->addPasses(
      m_graph, {.camera = camera, .items = m_visibleItems, .clearColor = clearColor}, output);
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

} // namespace chEngineSDK
