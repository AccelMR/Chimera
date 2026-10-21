/************************************************************************/
/**
 * @file chAnimationPreviewUI.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief Window that plays a model's animations away from the open scene.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chUUID.h"

namespace chEngineSDK {
class IAsset;
class ICommandList;

/**
 * Exists so a clip can be watched and stepped through without placing anything in the open
 * scene, as in Unreal's animation editor: a model, a clip, a skeleton or a dropped asset
 * opens a scene of its own with that model and two lights, drawn by its own SceneRenderer
 * into a texture the window shows. A clip plays on the first model of the project made for
 * its skeleton, and the window lists every model and clip that fits, so either can be
 * swapped. Drag to orbit, middle drag to pan, the wheel to zoom, F to frame. Nothing is
 * played or drawn while the window is hidden. The window stays open between sessions.
 *
 * A frame is render() while the UI is built, update() once the UI is done, then
 * recordFrame() in the render, before the pass that draws the UI.
 */
class AnimationPreviewUI
{
 public:
  static void
  render();

  /**
   * Plays the clip and copies the preview scene for the renderer.
   */
  static void
  update(float deltaTime);

  /**
   * Draws the preview into the texture the window showed this frame.
   */
  static void
  recordFrame(ICommandList& commandList);

  /**
   * True for the assets open() accepts: models, clips and skeletons.
   */
  NODISCARD static bool
  canOpen(const IAsset& asset);

  /**
   * Shows the asset (loading it first) and brings the window to the front.
   */
  static void
  open(const SPtr<IAsset>& asset);

  /**
   * Forgets the asset wherever the window uses it, before it is deleted.
   */
  static void
  close(const IAsset& asset);

  /**
   * Replaces outClips with the clips made for the skeleton, sorted by name; with a null
   * skeleton, the clips in the folder (rigid animations have no skeleton). Loads the clips,
   * which keep their skeleton inside.
   */
  static void
  findClips(const UUID& skeletonId, StringView folder, Vector<SPtr<IAsset>>& outClips);

  NODISCARD static bool
  isWindowVisible();

  static void
  setWindowVisible(bool bVisible);

  /**
   * Releases the preview scene, its renderer and its texture. Call after the GPU is idle,
   * before the asset manager and the graphics API shut down.
   */
  static void
  shutDown();
};

} // namespace chEngineSDK
