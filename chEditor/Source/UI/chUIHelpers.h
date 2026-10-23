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
  AnimationGraph,
  Script,
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

  /**
   * Makes the ImGui context with the editor's flags and theme and starts its platform
   * backend on display. iniFile must outlive the context (ImGui keeps the pointer); null
   * saves no layout. Returns the handle that feeds window events to ImGui.
   */
  NODISCARD static HEvent
  initializeImGui(const SPtr<DisplaySurface>& display,
                  const SPtr<DisplayEventHandle>& eventHandler,
                  const ANSICHAR* iniFile);

  /**
   * Stops the platform backend and destroys the context; the ImGui renderer must be gone
   * first, because it gives its textures back to the context.
   */
  static void
  shutDownImGui();

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

  /**
   * Opens the file in the program the system uses for its type (a .lua in the user's code
   * editor). Logs when it cannot.
   */
  static void
  openFileInSystem(const Path& file);

  /**
   * Opens a script in the code editor of Editor.ScriptEditor together with the project
   * folder, so its language server finds the script definitions; falls back to
   * openFileInSystem.
   */
  static void
  openScript(const Path& file);

  /**
   * The Editor.ScriptEditor field, for a menu.
   */
  static void
  renderScriptEditorSettings();

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
