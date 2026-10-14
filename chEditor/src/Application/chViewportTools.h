/************************************************************************/
/**
 * @file chViewportTools.h
 * @author AccelMR
 * @date 2026/10/13
 * @brief Selection, transform gizmos, light gizmos and the view cube of the viewport.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

class Camera;
class EditorCamera;
class Scene;
struct LightItem;

/**
 * Exists so the viewport edits the scene the way Unreal and Unity do: a click selects the
 * object under the mouse, a gizmo moves, rotates or scales the selection (W, E, R, or the
 * toolbar, which also picks world or local space and snapping), lights show an icon and the
 * selected one its reach, F frames the selection, and a cube in the corner turns the camera
 * to look along an axis. Everything is drawn with ImGui over the scene.
 */
class ViewportTools
{
 public:
  explicit ViewportTools(EditorCamera& editorCamera);

  ViewportTools(const ViewportTools&) = delete;

  ViewportTools&
  operator=(const ViewportTools&) = delete;

  /**
   * Call right after ImGui::NewFrame.
   */
  void
  beginFrame();

  /**
   * Draws the tools and applies what the user did to the scene and the camera. Call it
   * between beginFrame and ImGui::Render.
   */
  void
  render(Scene& scene);

  /**
   * True while a gizmo or the view cube is dragged; the camera must leave the mouse alone.
   */
  NODISCARD bool
  isUsingMouse() const;

 private:
  enum class Operation : uint32
  {
    Translate,
    Rotate,
    Scale
  };

  void
  handleShortcuts();

  void
  renderToolbar();

  void
  renderCameraSettings();

  void
  renderLights(const Camera& camera);

  void
  manipulateSelection(Scene& scene, const Camera& camera);

  void
  renderViewCube();

  void
  handleClickSelection(Scene& scene, const Camera& camera);

  /**
   * Frames the selected object with the camera.
   */
  void
  focusSelection();

  EditorCamera& m_editorCamera;

  Operation m_operation = Operation::Translate;
  bool m_bWorldSpace = true;
  bool m_bSnap = false;
  float m_translationSnap = 0.5f;
  float m_rotationSnapDegrees = 15.0f;
  float m_scaleSnap = 0.1f;

  // A click selects only when it started on the scene, not on a window or a gizmo.
  bool m_bClickStartedInScene = false;

  // Kept between frames, so finding the lights to draw does not allocate.
  Vector<const LightItem*> m_visibleLights;
};

} // namespace chEngineSDK
