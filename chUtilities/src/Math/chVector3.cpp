/************************************************************************/
/**
 * @file chVector3.cpp
 * @author AccelMR
 * @date 2021/11/01
 * @brief Vector for 3D spaces.
 *
 * A vector for 3D spaces composed by (X, Y, Z) with floating points
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chVector3.h"

#include "chVector2.h"
#include "chVector4.h"

namespace chEngineSDK {
 const Vector3 Vector3::ZERO = Vector3(0.0f, 0.0f, 0.0f);

 const Vector3 Vector3::UNIT = Vector3(1.0f, 1.0f, 1.0f);

 const Vector3 Vector3::RIGHT = Vector3(0.0f, 1.0f, 0.0f);
 const Vector3 Vector3::LEFT = Vector3::RIGHT*-1.0f;

 const Vector3 Vector3::UP = Vector3(0.0f, 0.0f, 1.0f);
 const Vector3 Vector3::DOWN = Vector3::UP*-1.0f;

 const Vector3 Vector3::FORWARD = Vector3(1.0f, 0.0f, 0.0f);
 const Vector3 Vector3::BACKWARD = Vector3::FORWARD*-1.0f;

/*
*/
Vector3::Vector3(const Vector2& _v2, float _z) : x(_v2.x), y(_v2.y), z(_z)
{}

/*
*/
Vector3::Vector3(const Vector4& v) : x(v.x), y(v.y), z(v.z)
{}
}


