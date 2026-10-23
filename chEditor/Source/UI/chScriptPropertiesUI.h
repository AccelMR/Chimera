/************************************************************************/
/**
 * @file chScriptPropertiesUI.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief Inspector rows for the properties a script lists.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class ScriptComponent;

/**
 * Exists so the values a script lists in its properties can be set per object without
 * opening the script. Each row edits an override of the component (saved with the scene, an
 * undo step); a reset button drops it and the script's default comes back. While the script
 * runs the rows show what self holds now, and an edit goes into self too. Values the script
 * no longer lists stay grayed out until removed.
 */
class ScriptPropertiesUI
{
 public:
  static void
  render(ScriptComponent& component);
};

} // namespace chEngineSDK
