/************************************************************************/
/**
 * @file chModelPreview.h
 * @author AccelMR
 * @date 2026/10/24
 * @brief A model in a scene of its own, drawn into a texture an editor window shows.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chCamera.h"
#include "chDebugDraw.h"

struct ImVec2;

namespace chEngineSDK {
class GameObject;
class ICommandList;
class ITexture;
class ITextureView;
class ModelAsset;
class ModelComponent;
class Scene;
class SceneRenderer;
class ViewportGrid;

/**
 * Exists so editor windows can show a model away from the open scene: a scene with the
 * model and two lights without shadows, an orbit camera (drag to orbit, middle drag to pan,
 * the wheel to zoom, F to frame), and a SceneRenderer of its own drawing into a texture the
 * window shows, with the grid and the bones on top. What animates the model is up to the
 * owner (an animator on getModelObject, or poses given to getModelComponent).
 *
 * A frame is renderView while the UI is built, update once the UI is done, then recordFrame
 * in the render, before the pass that draws the UI. Nothing is updated or drawn on frames
 * renderView did not show the image.
 */
class ModelPreview
{
 public:
  explicit ModelPreview(StringView name);
  ~ModelPreview();

  ModelPreview(const ModelPreview&) = delete;
  ModelPreview&
  operator=(const ModelPreview&) = delete;

  /**
   * Puts the model in the scene again (null removes it) and frames it.
   */
  void
  setModel(const SPtr<ModelAsset>& model);

  NODISCARD FORCEINLINE const SPtr<ModelAsset>&
  getModel() const noexcept
  {
    return m_model;
  }

  NODISCARD FORCEINLINE GameObject*
  getModelObject() const noexcept
  {
    return m_modelObject.get();
  }

  NODISCARD FORCEINLINE ModelComponent*
  getModelComponent() const noexcept
  {
    return m_component;
  }

  FORCEINLINE void
  requestFrame() noexcept
  {
    m_bFrameCamera = true;
  }

  /**
   * Draws the view at size: the image, or emptyText while no model is shown. The view is
   * the last ImGui item afterwards, so the caller can make it a drop target.
   */
  void
  renderView(const ImVec2& size, const ANSICHAR* emptyText);

  void
  update(float deltaTime);

  void
  recordFrame(ICommandList& commandList);

  /**
   * Releases the scene, the renderer and the texture. Call after the GPU is idle, before
   * the asset manager and the graphics API shut down.
   */
  void
  shutDown();

  /**
   * The preview settings every preview shares (bones, grid, light, camera speeds), saved to
   * the editor config when changed.
   */
  static void
  renderSettings();

 private:
  void
  createScene();

  void
  updateLights();

  void
  frameCamera();

  void
  handleCameraInput();

  void
  updateTarget(const ImVec2& size);

  String m_name;
  SPtr<ModelAsset> m_model;
  UniquePtr<Scene> m_scene;
  SPtr<GameObject> m_keyLight;
  SPtr<GameObject> m_fillLight;
  SPtr<GameObject> m_modelObject;
  ModelComponent* m_component = nullptr;
  Camera m_camera;
  bool m_bFrameCamera = false;

  UniquePtr<SceneRenderer> m_renderer;
  UniquePtr<ViewportGrid> m_grid;
  SPtr<ITexture> m_target;
  SPtr<ITextureView> m_targetView;
  uint32 m_targetWidth = 0;
  uint32 m_targetHeight = 0;
  DebugDraw m_lines;

  // The view showed the texture this frame, so update plays the scene and recordFrame
  // draws it; otherwise neither runs.
  bool m_bDrawn = false;
  bool m_bExtracted = false;
};

} // namespace chEngineSDK
