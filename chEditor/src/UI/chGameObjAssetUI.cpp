/************************************************************************/
/**
 * @file chGameObjAssetUI.cpp
 * @author  AccelMR
 * @date 2025/11/11
 * @brief   GameObject asset editor UI
 */
/************************************************************************/
#include "chGameObjAssetUI.h"

#include "chEditorSelection.h"
#include "chStringUtils.h"

#include "imgui.h"

namespace chEngineSDK {

void
GameObjectAssetUI::renderGameObjectAssetUI() {
  static bool windowOpen = false;
  SPtr<GameObject> gameObject = EditorSelection::getGameObjectAssetPreview();
  if (!gameObject) {
    return;
  }

  windowOpen = true;
  const String winTitle = chString::format("GameObject Asset Editor - {0}", gameObject->getName());
  ImGui::Begin(winTitle.c_str(), &windowOpen);

  // If the user closed the window (clicked the X), clear the preview selection so the editor won't reopen.
  if (!windowOpen) {
    EditorSelection::setGameObjectAssetPreview(nullptr);

    ImGui::End();
    return;
  }

  ImGui::Text("Editing GameObject Asset: %s", gameObject->getName().c_str());

  // Add more UI elements to edit the GameObject asset as needed

  ImGui::End();
}

} // namespace chEngineSDK
