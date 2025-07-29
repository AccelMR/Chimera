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

#include "chMultiStageRenderer.h"

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
  setMultiStageRenderer(SPtr<MultiStageRenderer> renderer) {
    m_multiStageRenderer = std::move(renderer);
  }

 private:
  //SPtr<NastyRenderer> m_nastyRenderer; ///< Renderer to set clear colors and
  SPtr<MultiStageRenderer> m_multiStageRenderer;
};
}  // namespace chEngineSDK
