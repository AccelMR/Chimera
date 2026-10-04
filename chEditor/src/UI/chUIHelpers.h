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
class IAssetCodec;

namespace chUIHelpers {
enum class AssetType { Model, Texture, Material, Shader, GameObject, Unknown };

struct AssetIcon
{
  AssetType type;
  const ANSICHAR* icon; // FontAwesome icon
};

class UIHelpers {
 public:

  static void
  newFrame(IGraphicsAPI& graphicAPI);

  static void
  render(IGraphicsAPI& graphicAPI, ICommandList& commandList);

  /**
   * Draws the ImGui windows dragged outside the main window. Runs after the main frame
   * is presented, because each of them submits and presents on its own.
   */
  static void
  renderPlatformWindows();

  NODISCARD static AssetIcon
  getIconFromAssetType(const SPtr<IAsset>& asset);

  static void
  initStyle();

  static void
  initFontConfig();

  NODISCARD static HEvent
  bindEventWindow(const SPtr<DisplayEventHandle>& eventHandler);

  static Path
  openFileExplorer(const Path& pathToOpen, const Vector<String>& filters = {});

  // Opens a file dialog filtered to the codec's extensions and imports the chosen file.
  // Returns nullptr if the dialog was cancelled or the import failed.
  static SPtr<IAsset>
  importAssetWithDialog(const SPtr<IAssetCodec>& codec);

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
