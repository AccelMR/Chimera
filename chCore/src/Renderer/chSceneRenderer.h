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

#include "chIRenderPath.h"
#include "chRenderGraph.h"
#include "chTransientTexturePool.h"

namespace chEngineSDK {

/**
 * Exists so an application only says where the frame goes and what it draws on top: it
 * owns the frame's render graph, the textures the graph makes, and the render path that
 * draws a scene. A frame is beginFrame(), imports and addScenePasses() (plus any passes of
 * the application, the editor UI), then execute().
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
   * Forgets the last frame's graph and returns it empty.
   */
  NODISCARD RenderGraph&
  beginFrame();

  /**
   * Adds the passes that draw what the camera sees of the scene into output.
   * Scene::updateTransforms must have run this frame, and the camera must stay alive until
   * execute().
   */
  void
  addScenePasses(const Scene& scene,
                 const Camera& camera,
                 RGTextureHandle output,
                 const LinearColor& clearColor);

  /**
   * Compiles the graph and records it into the command list of the frame.
   */
  void
  execute(ICommandList& commandList);

 private:
  RenderGraph m_graph;
  TransientTexturePool m_texturePool;
  UniquePtr<IRenderPath> m_renderPath;

  // Kept between frames, so gathering the visible items does not allocate.
  Vector<const RenderItem*> m_visibleItems;
};

} // namespace chEngineSDK
