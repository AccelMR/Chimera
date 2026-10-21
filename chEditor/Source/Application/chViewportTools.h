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
class DebugDraw;
class EditorCamera;
class ModelComponent;
class Scene;
struct LightItem;

/**
 * Where the scene image shows, in ImGui screen coordinates.
 */
struct ViewportArea
{
  float x = 0.0f;
  float y = 0.0f;
  float width = 1.0f;
  float height = 1.0f;
};

/**
 * Exists so the viewport edits the scene the way Unreal and Unity do: a click selects the
 * object under the mouse, a gizmo moves, rotates or scales the selection (W, E, R, or the
 * toolbar, which also picks world or local space and snapping), lights show an icon and the
 * selected one its reach, F frames the selection, and a cube in the corner turns the camera
 * to look along an axis. Everything is drawn with ImGui over the scene image of the
 * viewport panel.
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
   * Draws the tools over the scene image and applies what the user did to the scene and the
   * camera. Call it inside the viewport panel, after its image; bHovered says whether the
   * mouse is over the image and not over a window in front of it.
   */
  void
  render(Scene& scene, const ViewportArea& area, bool bHovered);

  /**
   * True while a gizmo or the view cube is dragged; the camera must leave the mouse alone.
   */
  NODISCARD bool
  isUsingMouse() const;

  /**
   * True while the mouse was over the transform gizmo last frame, or while it is dragged.
   */
  NODISCARD bool
  isGizmoHovered() const;

  /**
   * With Editor.ShowBounds on, adds the bounds of what the selected objects and their
   * children draw: the world box, and the mesh's own box turned with the object. Walks
   * every render item, so it costs nothing only while the setting is off.
   */
  static void
  addSelectionBounds(const Scene& scene, DebugDraw& outLines);

  NODISCARD static bool
  areBoundsVisible();

  /**
   * Also saves the choice in /Saved/Config/Editor.ini.
   */
  static void
  setBoundsVisible(bool bVisible);

  /**
   * With Editor.ShowSkeleton on, adds the bones of the models with a skeleton among the
   * selected objects and their children: a line from each bone to its parent and the axes
   * of each joint. Walks the selected subtrees, so it costs nothing while the setting is off.
   */
  static void
  addSelectionSkeletons(DebugDraw& outLines);

  /**
   * The bones of one model in its current pose, drawn the same way; nothing for a model
   * without a skeleton.
   */
  static void
  addSkeleton(const ModelComponent& component, DebugDraw& outLines);

  NODISCARD static bool
  areSkeletonsVisible();

  /**
   * Also saves the choice in /Saved/Config/Editor.ini.
   */
  static void
  setSkeletonsVisible(bool bVisible);

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
  renderCameras(const Scene& scene, const Camera& camera);

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

  // This frame's, set by render().
  ViewportArea m_area;
  bool m_bHovered = false;
  // Measured last frame, to center the toolbar.
  float m_toolbarWidth = 0.0f;

  // Kept between frames, so finding the lights to draw does not allocate.
  Vector<const LightItem*> m_visibleLights;
  // The objects a gizmo drag moves, refilled on each frame that changes them.
  Vector<SPtr<GameObject>> m_gizmoTargets;
};

} // namespace chEngineSDK
