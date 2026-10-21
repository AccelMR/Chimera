/************************************************************************/
/**
 * @file chLinearColor.h
 * @author AccelMR
 * @date 2022/09/21
 * @brief RGBA color with float components in linear space.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include <type_traits>

#include "chMath.h"

namespace chEngineSDK {
/**
 * Holds a color as four floats in linear space, the values shaders work with. Components
 * may go above 1 (light intensities, HDR clear colors).
 *
 * The class is not exported, so the constants are inline constexpr and other modules fold
 * them at compile time.
 */
class LinearColor
{
 public:
  /**
   * Leaves the values uninitialized, so a color filled right after is written once.
   * Use LinearColor::BLACK or LinearColor{} when it must start at a known value.
   */
  LinearColor() = default;

  FORCEINLINE constexpr
  LinearColor(float inR, float inG, float inB, float inA = 1.0f) noexcept;

  NODISCARD FORCEINLINE bool
  equals(const LinearColor& other, float tolerance = Math::KINDA_SMALL_NUMBER) const noexcept;

  NODISCARD FORCEINLINE float*
  getData() noexcept;

  NODISCARD FORCEINLINE const float*
  getData() const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator==(const LinearColor& other) const noexcept;

  NODISCARD FORCEINLINE constexpr LinearColor
  operator+(const LinearColor& other) const noexcept;

  NODISCARD FORCEINLINE constexpr LinearColor
  operator-(const LinearColor& other) const noexcept;

  NODISCARD FORCEINLINE constexpr LinearColor
  operator*(const LinearColor& other) const noexcept;

  NODISCARD FORCEINLINE constexpr LinearColor
  operator*(float scalar) const noexcept;

  FORCEINLINE constexpr LinearColor&
  operator+=(const LinearColor& other) noexcept;

  FORCEINLINE constexpr LinearColor&
  operator-=(const LinearColor& other) noexcept;

  FORCEINLINE constexpr LinearColor&
  operator*=(const LinearColor& other) noexcept;

  FORCEINLINE constexpr LinearColor&
  operator*=(float scalar) noexcept;

  NODISCARD static FORCEINLINE constexpr LinearColor
  lerp(const LinearColor& from, const LinearColor& to, float t) noexcept;

 public:
  // TRANSPARENT is a macro of <wingdi.h>.
  static const LinearColor TRANSPARENT_BLACK;
  static const LinearColor WHITE;
  static const LinearColor GRAY;
  static const LinearColor BLACK;
  static const LinearColor RED;
  static const LinearColor GREEN;
  static const LinearColor BLUE;
  static const LinearColor YELLOW;
  static const LinearColor CYAN;
  static const LinearColor MAGENTA;
  static const LinearColor LIGHT_GRAY;
  static const LinearColor DARK_GRAY;
  static const LinearColor PINK;
  static const LinearColor PURPLE;
  static const LinearColor LIGHT_BLUE;
  static const LinearColor YELLOW_GREEN;
  static const LinearColor ORANGE;
  static const LinearColor BROWN;
  static const LinearColor PALE_PINK;

  float r;
  float g;
  float b;
  float a;
};

static_assert(std::is_trivially_copyable_v<LinearColor>);
static_assert(sizeof(LinearColor) == 16);

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
LinearColor::LinearColor(float inR, float inG, float inB, float inA) noexcept
 : r(inR),
   g(inG),
   b(inB),
   a(inA)
{}

inline constexpr LinearColor LinearColor::TRANSPARENT_BLACK{0.0f, 0.0f, 0.0f, 0.0f};
inline constexpr LinearColor LinearColor::WHITE{1.0f, 1.0f, 1.0f, 1.0f};
inline constexpr LinearColor LinearColor::GRAY{0.5f, 0.5f, 0.5f, 1.0f};
inline constexpr LinearColor LinearColor::BLACK{0.0f, 0.0f, 0.0f, 1.0f};
inline constexpr LinearColor LinearColor::RED{1.0f, 0.0f, 0.0f, 1.0f};
inline constexpr LinearColor LinearColor::GREEN{0.0f, 1.0f, 0.0f, 1.0f};
inline constexpr LinearColor LinearColor::BLUE{0.0f, 0.0f, 1.0f, 1.0f};
inline constexpr LinearColor LinearColor::YELLOW{1.0f, 1.0f, 0.0f, 1.0f};
inline constexpr LinearColor LinearColor::CYAN{0.0f, 1.0f, 1.0f, 1.0f};
inline constexpr LinearColor LinearColor::MAGENTA{1.0f, 0.0f, 1.0f, 1.0f};
inline constexpr LinearColor LinearColor::LIGHT_GRAY{0.878f, 0.878f, 0.878f, 1.0f};
inline constexpr LinearColor LinearColor::DARK_GRAY{0.25f, 0.25f, 0.25f, 1.0f};
inline constexpr LinearColor LinearColor::PINK{1.0f, 0.37f, 0.8156f, 1.0f};
inline constexpr LinearColor LinearColor::PURPLE{0.6274f, 0.1254f, 1.0f, 1.0f};
inline constexpr LinearColor LinearColor::LIGHT_BLUE{0.313f, 0.8156f, 1.0f, 1.0f};
inline constexpr LinearColor LinearColor::YELLOW_GREEN{0.375f, 1.0f, 0.5f, 1.0f};
inline constexpr LinearColor LinearColor::ORANGE{1.0f, 0.625f, 0.0627f, 1.0f};
inline constexpr LinearColor LinearColor::BROWN{0.625f, 0.5f, 0.375f, 1.0f};
inline constexpr LinearColor LinearColor::PALE_PINK{0.8945f, 0.7265f, 0.9375f, 1.0f};

/*
 */
FORCEINLINE bool
LinearColor::equals(const LinearColor& other, float tolerance) const noexcept
{
  return Math::abs(r - other.r) <= tolerance && Math::abs(g - other.g) <= tolerance &&
         Math::abs(b - other.b) <= tolerance && Math::abs(a - other.a) <= tolerance;
}

/*
 */
FORCEINLINE float*
LinearColor::getData() noexcept
{
  return &r;
}

/*
 */
FORCEINLINE const float*
LinearColor::getData() const noexcept
{
  return &r;
}

/*
 */
FORCEINLINE constexpr bool
LinearColor::operator==(const LinearColor& other) const noexcept
{
  return r == other.r && g == other.g && b == other.b && a == other.a;
}

/*
 */
FORCEINLINE constexpr LinearColor
LinearColor::operator+(const LinearColor& other) const noexcept
{
  return {r + other.r, g + other.g, b + other.b, a + other.a};
}

/*
 */
FORCEINLINE constexpr LinearColor
LinearColor::operator-(const LinearColor& other) const noexcept
{
  return {r - other.r, g - other.g, b - other.b, a - other.a};
}

/*
 */
FORCEINLINE constexpr LinearColor
LinearColor::operator*(const LinearColor& other) const noexcept
{
  return {r * other.r, g * other.g, b * other.b, a * other.a};
}

/*
 */
FORCEINLINE constexpr LinearColor
LinearColor::operator*(float scalar) const noexcept
{
  return {r * scalar, g * scalar, b * scalar, a * scalar};
}

/*
 */
FORCEINLINE constexpr LinearColor&
LinearColor::operator+=(const LinearColor& other) noexcept
{
  r += other.r;
  g += other.g;
  b += other.b;
  a += other.a;
  return *this;
}

/*
 */
FORCEINLINE constexpr LinearColor&
LinearColor::operator-=(const LinearColor& other) noexcept
{
  r -= other.r;
  g -= other.g;
  b -= other.b;
  a -= other.a;
  return *this;
}

/*
 */
FORCEINLINE constexpr LinearColor&
LinearColor::operator*=(const LinearColor& other) noexcept
{
  r *= other.r;
  g *= other.g;
  b *= other.b;
  a *= other.a;
  return *this;
}

/*
 */
FORCEINLINE constexpr LinearColor&
LinearColor::operator*=(float scalar) noexcept
{
  r *= scalar;
  g *= scalar;
  b *= scalar;
  a *= scalar;
  return *this;
}

/*
 */
FORCEINLINE constexpr LinearColor
LinearColor::lerp(const LinearColor& from, const LinearColor& to, float t) noexcept
{
  return {from.r + (to.r - from.r) * t, from.g + (to.g - from.g) * t,
          from.b + (to.b - from.b) * t, from.a + (to.a - from.a) * t};
}

} // namespace chEngineSDK
