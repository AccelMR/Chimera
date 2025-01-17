/************************************************************************/
/**
 * @file chVector2.cpp
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/16
 * @brief Vector for 2D spaces.
 *
 * A vector for 2D spaces composed by (X,Y) with floating points.
 *
 * @bug No bug known.
 */
 /************************************************************************/

 /************************************************************************/
 /*
  * Includes.
  */
  /************************************************************************/
#include "chVector2.h"

#include "chVector3.h"

namespace chEngineSDK {

/*
*/
Vector2::Vector2(const Vector3& vector3)
  : x(vector3.x),
    y(vector3.y)
{}

const Vector2 Vector2::ZERO = Vector2(0.0f, 0.0f);
const Vector2 Vector2::UNIT = Vector2(1.0f, 1.0f);
const Vector2 Vector2::UNIT_X = Vector2(1.0f, 0.0f);
const Vector2 Vector2::UNIT_Y = Vector2(0.0f, 1.0f);

}

