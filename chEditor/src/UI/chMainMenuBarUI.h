/************************************************************************/
/**
 * @file chMainMenuBar.h
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class NastyRenderer;

class MainMenuBarUI {
 public:
  MainMenuBarUI() = default;
  ~MainMenuBarUI() = default;

  void
  renderMainMenuBar();

  void
  renderImportMenu();

  // DELETEME
  void
  setNastyRenderer(const SPtr<NastyRenderer>& renderer) {
    m_nastyRenderer = renderer;
  }

 private:
  SPtr<NastyRenderer> m_nastyRenderer; ///< Renderer to set clear colors and
};
}  // namespace chEngineSDK
