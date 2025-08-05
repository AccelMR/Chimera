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
 private:
};
}  // namespace chEngineSDK
