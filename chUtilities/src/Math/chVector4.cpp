/************************************************************************/
/**
 * @file chVector4.cpp
 * @author AccelMR
 * @date 2022/02/15
 *   Vector 4D spaced composed by ( X, Y, Z, W ) with floating points.
 *
 */
 /************************************************************************/

 /************************************************************************/
 /*
  * Includes.
  */                                                                     
 /************************************************************************/
#include "chVector4.h"

#include "chVector3.h"

namespace chEngineSDK{

const Vector4 Vector4::ZERO = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

const Vector4 Vector4::UNIT = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

/*
*/
Vector4::Vector4(const Vector3& vector3, const float& _w)
  : x(vector3.x),
    y(vector3.y),
    z(vector3.z),
    w(_w)
{}
}
