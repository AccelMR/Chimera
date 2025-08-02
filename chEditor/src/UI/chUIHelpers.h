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
enum class AssetType { Model, Texture, Material, Shader, Unknown };

struct AssetIcon {
  AssetType type;
  const char* icon; // FontAwesome icon name
  const String IconName = "";
};

class UIHelpers {
 public:

  static void
  newFrame(IGraphicsAPI& graphicAPI);

  static void
  render(IGraphicsAPI& graphicAPI, const SPtr<ICommandBuffer>& commandBuffer);

  static AssetIcon
  getIconFromAssetType(const SPtr<IAsset>& asset);

  static void
  initStyle();

  static void
  initFontConfig();

  NODISCARD static HEvent
  bindEventWindow(const SPtr<DisplayEventHandle>& eventHandler);

  static Path
  openFileExplorer(const Path& pathToOpen, const Vector<String>& filters = {});

  // Variable to control the visibility of the ImGui demo window
  static bool bShowDemoWindow;
  // Variable to control if ImGui should render
  static bool bRenderImGui;
  static float baseFontSize; // Default font size for ImGui
  static LinearColor backgroundColor; // Background color for the editor
  static LinearColor rendererColor; // Renderer color for the editor
}; // class UIHelpers

} // namespace chUIHelpers
} // namespace chEngineSDK
