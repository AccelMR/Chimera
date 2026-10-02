/************************************************************************/
/**
 * @file chGameObjAssetUI.h
 * @author  AccelMR
 * @date 2025/11/11
 * @brief   GameObject asset editor UI
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class GameObjectAssetUI {
 public:
  GameObjectAssetUI() = default;
  ~GameObjectAssetUI() = default;

  void
  renderGameObjectAssetUI();

 private:
  // Rebuilt only when the edited object's name changes.
  String m_windowTitle;
  String m_titleName;
};
} // namespace chEngineSDK
