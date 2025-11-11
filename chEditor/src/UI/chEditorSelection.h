/************************************************************************/
/**
 * @file chEditorSelection.h
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Selection management for the Chimera Editor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chUUID.h"
#include "chGameObject.h"

namespace chEngineSDK {

class EditorSelection {
 public:
  static SPtr<GameObject>
  getSelectedGameObject() {
    return m_selectedGameObject;
  }

  static void
  setSelectedGameObject(SPtr<GameObject> gameObject) {
    m_selectedGameObject = std::move(gameObject);
  }

 private:
  static SPtr<GameObject> m_selectedGameObject;
};

} // namespace chEngineSDK
