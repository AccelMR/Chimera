/************************************************************************/
/**
 * @file chCameraPreviewUI.h
 * @author AccelMR
 * @date 2026/10/25
 * @brief What a camera of the open scene sees, shown in the editor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class CameraComponent;
class GameObject;
class ICommandList;

/**
 * Exists so a camera can be placed without starting Play: the Object Inspector shows a
 * small image of what the selected camera sees (no shadows or ambient occlusion unless the
 * Editor.CameraPreview* settings ask for them), and the Camera Preview window shows it big
 * with the viewport's quality. Both keep the shape of the editor's window, the size a game
 * starts at. Piloting puts the viewport behind a camera: flying the editor camera moves the
 * camera's object, one undo step per move.
 *
 * One SceneRenderer draws at most one of the two images per frame: while the window shows
 * the selected camera, the Inspector says so instead of drawing it again. Nothing is drawn
 * during Play, while the camera is piloted, or while neither image is on screen.
 *
 * A frame is render() and renderInspectorView() while the UI is built, update() once the
 * scene is updated, then recordFrame() in the render, before the pass that draws the UI.
 */
class CameraPreviewUI
{
 public:
  /**
   * The Camera Preview window, when shown.
   */
  static void
  render();

  /**
   * The image of the camera of object, Pilot and the button that opens the window, inside
   * the Inspector's Camera section.
   */
  static void
  renderInspectorView(const SPtr<GameObject>& object);

  /**
   * Copies the open scene, seen by the camera of the image drawn this frame.
   */
  static void
  update();

  static void
  recordFrame(ICommandList& commandList);

  /**
   * Shows the window on the camera of gameObject.
   */
  static void
  open(const SPtr<GameObject>& gameObject);

  static void
  startPiloting(const SPtr<GameObject>& gameObject);

  static void
  stopPiloting();

  /**
   * The camera the viewport looks through, null when none is piloted or its object left
   * the open scene.
   */
  NODISCARD static CameraComponent*
  getPilotedCamera();

  NODISCARD static bool
  isWindowVisible();

  static void
  setWindowVisible(bool bVisible);

  /**
   * Releases the renderer and the textures. Call after the GPU is idle.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
