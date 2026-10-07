/************************************************************************/
/**
 * @file chPlatformMath.cpp
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 * @brief Scalar math functions that need <cmath>.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPlatformMath.h"

#include <cmath>

#include "chAngle.h"

namespace chEngineSDK {

#if !USING(CH_MATH_SQRT_BUILTIN) && !USING(CH_MATH_SQRT_SSE)
/*
 */
float
PlatformMath::sqrtNoIntrinsic(float value) noexcept
{
  return std::sqrt(value);
}
#endif

/*
 */
float
PlatformMath::pow(float value, float exponent) noexcept
{
  return std::pow(value, exponent);
}

/*
 */
float
PlatformMath::fmod(float value, float divisor) noexcept
{
  return std::fmod(value, divisor);
}

/*
 */
float
PlatformMath::cos(const Radian& angle) noexcept
{
  return std::cos(angle.valueRadian());
}

/*
 */
float
PlatformMath::cos(const Degree& angle) noexcept
{
  return std::cos(angle.valueRadian());
}

/*
 */
float
PlatformMath::sin(const Radian& angle) noexcept
{
  return std::sin(angle.valueRadian());
}

/*
 */
float
PlatformMath::sin(const Degree& angle) noexcept
{
  return std::sin(angle.valueRadian());
}

/*
 */
float
PlatformMath::tan(const Radian& angle) noexcept
{
  return std::tan(angle.valueRadian());
}

/*
 */
float
PlatformMath::tan(const Degree& angle) noexcept
{
  return std::tan(angle.valueRadian());
}

/*
 */
Radian
PlatformMath::acos(float value) noexcept
{
  return Radian(std::acos(clamp(value, -1.0f, 1.0f)));
}

/*
 */
Radian
PlatformMath::asin(float value) noexcept
{
  return Radian(std::asin(clamp(value, -1.0f, 1.0f)));
}

/*
 */
Radian
PlatformMath::atan(float value) noexcept
{
  return Radian(std::atan(value));
}

/*
 */
Radian
PlatformMath::atan2(float y, float x) noexcept
{
  return Radian(std::atan2(y, x));
}

/*
 */
float
PlatformMath::cosh(float value) noexcept
{
  return std::cosh(value);
}

/*
 */
float
PlatformMath::sinh(float value) noexcept
{
  return std::sinh(value);
}

/*
 */
float
PlatformMath::tanh(float value) noexcept
{
  return std::tanh(value);
}

/*
 */
float
PlatformMath::acosh(float value) noexcept
{
  return std::acosh(value);
}

/*
 */
float
PlatformMath::asinh(float value) noexcept
{
  return std::asinh(value);
}

/*
 */
float
PlatformMath::atanh(float value) noexcept
{
  return std::atanh(value);
}
} // namespace chEngineSDK
