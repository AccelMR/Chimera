/************************************************************************/
/**
 * @file chInspectorUI.cpp
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Inspector UI class for the Chimera Editor.
 */
/************************************************************************/

#include "chInspectorUI.h"

#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chLogger.h"

#include "imgui.h"

CH_LOG_DECLARE_STATIC(InspectorUILog, All);

namespace chEngineSDK {

InspectorUI::InspectorUI() {
  // Constructor implementation
}

InspectorUI::~InspectorUI() {
  // Destructor implementation
}

void
InspectorUI::renderInspectorUI() {
  ImGui::Begin("Inspector", nullptr);
  SPtr<GameObject> selectedGameObject = EditorSelection::getSelectedGameObject();
  if (!selectedGameObject) {
    ImGui::End();
    return; // No GameObject selected, nothing to render
  }
  ImGui::Text("Selected GameObject: %s", selectedGameObject->getName().c_str());
  // Rendering code for the inspector UI
  ImGui::End();
}

} // namespace chEngineSDK
