/************************************************************************/
/**
 * @file UIHelpers.h
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chEventSystem.h"

namespace chEngineSDK {

namespace chUIHelpers {
enum class AssetType
{
  Model,
  Texture,
  Material,
  Shader,
  Prefab,
  Scene,
  Skeleton,
  Animation,
  Unknown
};

struct AssetIcon
{
  AssetType type;
  const ANSICHAR* icon; // FontAwesome icon
};

class UIHelpers {
 public:

  static void
  newFrame();

  NODISCARD static AssetIcon
  getIconFromAssetType(const SPtr<IAsset>& asset);

  static void
  initConfigFlags();

  NODISCARD static HEvent
  bindEventWindow(const SPtr<DisplayEventHandle>& eventHandler);

  NODISCARD static const ANSICHAR*
  getFolderIcon(bool bOpen);

  /**
   * A box sized to its contents over the current window (the viewport's toolbar and stats),
   * at the cursor, with the window background at backgroundAlpha. windowFlags are
   * ImGuiWindowFlags. Call endOverlayChild() whatever it returns.
   */
  NODISCARD static bool
  beginOverlayChild(const ANSICHAR* id, float backgroundAlpha, int32 windowFlags);

  static void
  endOverlayChild();

  // Variable to control the visibility of the ImGui demo window
  static bool bShowDemoWindow;
  // False hides the whole editor UI (F11), leaving only the scene.
  static bool bRenderImGui;
  // Set by View > Reset Layout; the editor docks the panels in their default places again.
  static bool bResetLayout;
  static LinearColor rendererColor; // Renderer color for the editor
}; // class UIHelpers

} // namespace chUIHelpers
} // namespace chEngineSDK
