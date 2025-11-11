/************************************************************************/
/**
 * @file chInspectorUI.h
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Inspector UI class for the Chimera Editor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGameObject.h"

namespace chEngineSDK {

class InspectorUI {
 public:
  InspectorUI();
  ~InspectorUI();

  void
  renderInspectorUI();

 private:
};

} // namespace chEngineSDK
