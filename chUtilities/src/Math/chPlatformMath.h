/************************************************************************/
/**
 * @file chPlatformMath.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 * @brief Scalar math functions and constants for the engine, in float only.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include <bit>

#if USING(CH_COMPILER_CLANG) || USING(CH_COMPILER_GNUC)
# define CH_MATH_SQRT_BUILTIN IN_USE
#elif USING(CH_ARCHITECTURE_X86_64)
# define CH_MATH_SQRT_SSE IN_USE
# include <xmmintrin.h>
#endif

#if !defined(CH_MATH_SQRT_BUILTIN)
# define CH_MATH_SQRT_BUILTIN NOT_IN_USE
#endif
#if !defined(CH_MATH_SQRT_SSE)
# define CH_MATH_SQRT_SSE NOT_IN_USE
#endif

namespace chEngineSDK {
/**
 * Gives the engine one place for scalar math, so the hot functions (abs, sqrt, min, max,
 * clamp...) are inline everywhere without pulling <cmath> into every file, and the
 * constants are known at compile time in every module.
 */
class PlatformMath
{
 public:
  PlatformMath() = delete;

  /************************************************************************/
  /*
   * Algebra.
   */
  /************************************************************************/
  NODISCARD static FORCEINLINE float
  sqrt(float value) noexcept
  {
#if USING(CH_MATH_SQRT_BUILTIN)
    return __builtin_sqrtf(value);
#elif USING(CH_MATH_SQRT_SSE)
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(value)));
#else
    return sqrtNoIntrinsic(value);
#endif
  }

  NODISCARD static FORCEINLINE float
  invSqrt(float value) noexcept
  {
    return 1.0f / sqrt(value);
  }

  NODISCARD static CH_UTILITY_EXPORT float
  pow(float value, float exponent) noexcept;

  template<class T>
  NODISCARD static FORCEINLINE constexpr T
  square(T value) noexcept
  {
    return value * value;
  }

  NODISCARD static FORCEINLINE constexpr float
  abs(float value) noexcept
  {
    return std::bit_cast<float>(std::bit_cast<uint32>(value) & 0x7FFFFFFFu);
  }

  /**
   * False for NaN and infinity. Reads the exponent bits, so it still works when the
   * compiler is told to assume finite math.
   */
  NODISCARD static FORCEINLINE constexpr bool
  isFinite(float value) noexcept
  {
    return (std::bit_cast<uint32>(value) & 0x7F800000u) != 0x7F800000u;
  }

  template<class T>
  NODISCARD static FORCEINLINE constexpr T
  min(T a, T b) noexcept
  {
    return a < b ? a : b;
  }

  template<class T>
  NODISCARD static FORCEINLINE constexpr T
  max(T a, T b) noexcept
  {
    return a > b ? a : b;
  }

  template<class T>
  NODISCARD static FORCEINLINE constexpr T
  clamp(T value, T low, T high) noexcept
  {
    return value < low ? low : value < high ? value : high;
  }

  NODISCARD static FORCEINLINE constexpr float
  lerp(float from, float to, float alpha) noexcept
  {
    return from + alpha * (to - from);
  }

  /**
   * Returns the alpha that lerp(from, to, alpha) needs to give value. from and to must
   * differ.
   */
  NODISCARD static FORCEINLINE constexpr float
  invLerp(float from, float to, float value) noexcept
  {
    return (value - from) / (to - from);
  }

  NODISCARD static FORCEINLINE constexpr bool
  nearEqual(float a, float b, float tolerance = SMALL_NUMBER) noexcept
  {
    return abs(a - b) <= tolerance;
  }

  /**
   * Rounds value up to the next multiple of alignment, which must be above 0.
   */
  NODISCARD static constexpr uint64
  alignUp(uint64 value, uint64 alignment) noexcept
  {
    return (value + alignment - 1) / alignment * alignment;
  }

  NODISCARD static CH_UTILITY_EXPORT float
  fmod(float value, float divisor) noexcept;

  /************************************************************************/
  /*
   * Trigonometry.
   */
  /************************************************************************/
  NODISCARD static CH_UTILITY_EXPORT float
  cos(const Radian& angle) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  cos(const Degree& angle) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  sin(const Radian& angle) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  sin(const Degree& angle) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  tan(const Radian& angle) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  tan(const Degree& angle) noexcept;

  /**
   * Sine and cosine of an angle in radians in one call, with polynomials accurate to about
   * 1e-7. Faster than sin plus cos and inline.
   */
  static FORCEINLINE void
  sinCos(float radians, float& outSin, float& outCos) noexcept;

  /**
   * The input is clamped to [-1, 1], so rounding error (a dot product of unit vectors
   * giving 1.0000001) returns 0 instead of NaN.
   */
  NODISCARD static CH_UTILITY_EXPORT Radian
  acos(float value) noexcept;

  /**
   * The input is clamped to [-1, 1], like acos.
   */
  NODISCARD static CH_UTILITY_EXPORT Radian
  asin(float value) noexcept;

  NODISCARD static CH_UTILITY_EXPORT Radian
  atan(float value) noexcept;

  NODISCARD static CH_UTILITY_EXPORT Radian
  atan2(float y, float x) noexcept;

  /**
   * Wraps the angle into [-180, 180].
   */
  NODISCARD static FORCEINLINE float
  unwindDegrees(float degrees) noexcept;

  /**
   * Wraps the angle into [-PI, PI].
   */
  NODISCARD static FORCEINLINE float
  unwindRadians(float radians) noexcept;

  /************************************************************************/
  /*
   * Hyperbolic functions.
   */
  /************************************************************************/
  NODISCARD static CH_UTILITY_EXPORT float
  cosh(float value) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  sinh(float value) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  tanh(float value) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  acosh(float value) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  asinh(float value) noexcept;

  NODISCARD static CH_UTILITY_EXPORT float
  atanh(float value) noexcept;

  /************************************************************************/
  /*
   * Constants.
   */
  /************************************************************************/
  static constexpr float PI = 3.14159265358979323846f;
  static constexpr float TWO_PI = 6.28318530717958647692f;
  static constexpr float HALF_PI = 1.57079632679489661923f;
  static constexpr float QUARTER_PI = 0.78539816339744830962f;
  static constexpr float INV_PI = 0.31830988618379067154f;
  static constexpr float RAD2DEG = 57.2957795130823208768f;
  static constexpr float DEG2RAD = 0.01745329251994329577f;
  static constexpr float SMALL_NUMBER = 1.e-6f;
  static constexpr float KINDA_SMALL_NUMBER = 1.e-4f;

 private:
#if !USING(CH_MATH_SQRT_BUILTIN) && !USING(CH_MATH_SQRT_SSE)
  // Compilers with no sqrt intrinsic (MSVC on ARM64) call <cmath> from the .cpp, so the
  // header never includes it.
  NODISCARD static CH_UTILITY_EXPORT float
  sqrtNoIntrinsic(float value) noexcept;
#endif
};

/************************************************************************/
/*
 * Implementations.
 */
/************************************************************************/

/*
 */
FORCEINLINE float
PlatformMath::unwindDegrees(float degrees) noexcept
{
  if (degrees > 180.0f || degrees < -180.0f) {
    // fmod is exact, so adding half a turn before it would only add rounding error.
    degrees = fmod(degrees, 360.0f);
    if (degrees > 180.0f) {
      degrees -= 360.0f;
    }
    else if (degrees < -180.0f) {
      degrees += 360.0f;
    }
  }
  return degrees;
}

/*
 */
FORCEINLINE float
PlatformMath::unwindRadians(float radians) noexcept
{
  if (radians > PI || radians < -PI) {
    radians = fmod(radians, TWO_PI);
    if (radians > PI) {
      radians -= TWO_PI;
    }
    else if (radians < -PI) {
      radians += TWO_PI;
    }
  }
  return radians;
}

/*
 */
FORCEINLINE void
PlatformMath::sinCos(float radians, float& outSin, float& outCos) noexcept
{
  // Map the angle to y in [-PI, PI]: radians = 2 * PI * quotient + y.
  float y;
  float quotient = (INV_PI * 0.5f) * radians;
  if (abs(quotient) < 8388608.0f) {
    quotient = static_cast<float>(
        static_cast<int32>(quotient >= 0.0f ? quotient + 0.5f : quotient - 0.5f));
    y = radians - TWO_PI * quotient;
  }
  else {
    // Casting a quotient this big (or NaN) to an integer is undefined, and the subtraction
    // above would lose every digit; fmod is exact at any size.
    y = unwindRadians(radians);
  }

  // Map y to [-PI / 2, PI / 2] with sin(y) = sin(radians).
  float sign = 1.0f;
  if (y > HALF_PI) {
    y = PI - y;
    sign = -1.0f;
  }
  else if (y < -HALF_PI) {
    y = -PI - y;
    sign = -1.0f;
  }

  const float y2 = y * y;

  // 11-degree minimax approximation.
  outSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 +
              0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

  // 10-degree minimax approximation.
  const float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 +
                    0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
  outCos = sign * p;
}
} // namespace chEngineSDK
