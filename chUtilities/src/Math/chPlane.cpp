/************************************************************************/
/**
 * @file chPlane.cpp
 * @author AccelMR
 * @date 2025/05/01
 * @brief   Way to represent three dimensional planes.
 *
 *  Three dimensional plane.
 */
/************************************************************************/

#include "chPlane.h"

#include "chVector4.h"

namespace chEngineSDK{

/*
*/
Plane::Plane(const Vector4& v) : Vector3(v), w(v.w) {}

} // namespace chEngineSDK
