/************************************************************************/
/**
 * @file chScriptMathTypes.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  The math types scripts get as values.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chLinearColor.h"
#include "chQuaternion.h"
#include "chRotator.h"
#include "chScriptBinder.h"
#include "chVector2.h"
#include "chVector3.h"
#include "chVector4.h"

namespace chEngineSDK {

// Their names in scripts. Bindings of other types include this header to take or return them.
CH_SCRIPT_VALUE_TYPE(Vector2, "Vector2");
CH_SCRIPT_VALUE_TYPE(Vector3, "Vector3");
CH_SCRIPT_VALUE_TYPE(Vector4, "Vector4");
CH_SCRIPT_VALUE_TYPE(Quaternion, "Quaternion");
CH_SCRIPT_VALUE_TYPE(Rotator, "Rotator");
CH_SCRIPT_VALUE_TYPE(LinearColor, "Color");

} // namespace chEngineSDK
