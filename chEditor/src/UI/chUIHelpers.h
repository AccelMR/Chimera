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
enum class AssetType { Model, Texture, Material, Shader, Prefab, Scene, Skeleton, Unknown };

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

  // Variable to control the visibility of the ImGui demo window
  static bool bShowDemoWindow;
  // False hides the whole editor UI (F11), leaving only the scene.
  static bool bRenderImGui;
  static LinearColor rendererColor; // Renderer color for the editor
}; // class UIHelpers

} // namespace chUIHelpers
} // namespace chEngineSDK
