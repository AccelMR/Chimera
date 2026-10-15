/************************************************************************/
/**
 * @file chEditorTheme.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief
 *  Colors, sizes and font of the editor UI.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

// Saved by name in Editor.ini, so the values can be reordered.
enum class EditorThemeType : uint8 { Classic, CatppuccinMocha, COUNT };

enum class EditorFontType : uint8 { JetBrainsMono, Inter, FiraCode, COUNT };

/**
 * Exists so the look of the editor can be changed from the menu while it runs and is kept
 * between sessions, without every panel knowing which theme or font is in use.
 */
class EditorTheme
{
 public:
  /**
   * Loads every font and applies the saved settings. Call once, after the ImGui context is
   * made and before the first frame.
   */
  static void
  initialize();

  /**
   * Applies the changes the menu asked for. Call before ImGui::NewFrame: ImGui reads the
   * style and the default font when a frame starts.
   */
  static void
  applyPending();

  /**
   * Draws the theme, font, size and opacity choices. Meant for the inside of a menu.
   */
  static void
  renderSettings();
};

} // namespace chEngineSDK
