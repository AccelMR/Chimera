/************************************************************************/
/**
 * @file chLinearColor.h
 * @author AccelMR
 * @date 2022/09/21
 *   Linear color class that holds a color as 4 bytes normalized float RGBA
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

#include "chMath.h"

namespace chEngineSDK {
/*
 * Description:
 *     Linear color class that holds a float normalized color.
 *     Each component is in the range [0.0, 1.0]
 *     E.g. Red = 1.0f, 0.0f, 0.0f, 1.0f
 *
 * Sample usage:
 * LinearColor white = LinearColor::White;
 * LinearColor gray = white * LinearColor(0.5f, 0.5f, 0.5f, 1.0f);
 */
class CH_UTILITY_EXPORT LinearColor
{
 public:
  /*
   * @brief Default constructor - initializes to black
   */
  FORCEINLINE
  LinearColor();

  /**
   * @brief Constructor from 3 or 4 floats
   * Components should be in range [0.0, 1.0]
   *
   * @param _r Red component
   * @param _g Green component
   * @param _b Blue component
   * @param _a Alpha component (defaults to 1.0 - fully opaque)
   */
  FORCEINLINE
  LinearColor(float _r, float _g, float _b, float _a = 1.0f);

  /**
   * @brief Constructor from Color (8-bit per channel)
   * Converts from 0-255 range to 0.0-1.0 range
   *
   * @param color The 8-bit per channel color to convert
   */
  constexpr LinearColor(const Color& color);

  /*
   * @brief Default destructor
   */
  ~LinearColor() = default;

  /**
   * @brief Returns true if color is nearly equal to another with given tolerance
   *
   * @param linearColor Color to check against
   * @param tolerance The tolerance for comparison (default is KINDA_SMALL_NUMBER)
   * @return bool True if colors are nearly equal
   */
  NODISCARD FORCEINLINE bool
  equals(const LinearColor& linearColor, float tolerance = Math::KINDA_SMALL_NUMBER) const;

  /**
   * @brief True if both colors are exactly equal
   *
   * @param linearColor Color to compare with
   * @return bool True if colors are exactly equal
   */
  NODISCARD FORCEINLINE bool
  operator==(const LinearColor& linearColor) const;

  /**
   * @brief True if both colors are different
   *
   * @param linearColor Color to compare with
   * @return bool True if colors are different
   */
  NODISCARD FORCEINLINE bool
  operator!=(const LinearColor& linearColor) const;

  /**
   * @brief Adds two colors component-wise
   * Note: This may produce values outside the [0,1] range
   *
   * @param linearColor Color to add
   * @return LinearColor Result of addition
   */
  NODISCARD FORCEINLINE LinearColor
  operator+(const LinearColor& linearColor) const;

  /**
   * @brief Adds another color to this one component-wise
   * Note: This may produce values outside the [0,1] range
   *
   * @param linearColor Color to add
   * @return LinearColor& Reference to this
   */
  FORCEINLINE LinearColor&
  operator+=(const LinearColor& linearColor);

  /**
   * @brief Subtracts two colors component-wise
   * Note: This may produce values outside the [0,1] range
   *
   * @param linearColor Color to subtract
   * @return LinearColor Result of subtraction
   */
  NODISCARD FORCEINLINE LinearColor
  operator-(const LinearColor& linearColor) const;

  /**
   * @brief Subtracts another color from this one component-wise
   * Note: This may produce values outside the [0,1] range
   *
   * @param linearColor Color to subtract
   * @return LinearColor& Reference to this
   */
  FORCEINLINE LinearColor&
  operator-=(const LinearColor& linearColor);

  /**
   * @brief Multiplies two colors component-wise
   * This is the proper way to blend colors in linear space
   *
   * @param linearColor Color to multiply with
   * @return LinearColor Result of multiplication
   */
  NODISCARD FORCEINLINE LinearColor
  operator*(const LinearColor& linearColor) const;

  /**
   * @brief Multiplies this color by another component-wise
   * This is the proper way to blend colors in linear space
   *
   * @param linearColor Color to multiply with
   * @return LinearColor& Reference to this
   */
  FORCEINLINE LinearColor&
  operator*=(const LinearColor& linearColor);

  /**
   * @brief Divides two colors component-wise
   * Note: Division by zero is not checked
   *
   * @param linearColor Color to divide by
   * @return LinearColor Result of division
   */
  NODISCARD FORCEINLINE LinearColor
  operator/(const LinearColor& linearColor) const;

  /**
   * @brief Divides this color by another component-wise
   * Note: Division by zero is not checked
   *
   * @param linearColor Color to divide by
   * @return LinearColor& Reference to this
   */
  FORCEINLINE LinearColor&
  operator/=(const LinearColor& linearColor);

  /**
   * @brief Multiplies all components by a scalar
   *
   * @param scalar Scalar value to multiply by
   * @return LinearColor Result of multiplication
   */
  NODISCARD FORCEINLINE LinearColor
  operator*(float scalar) const;

  /**
   * @brief Multiplies all components by a scalar
   *
   * @param scalar Scalar value to multiply by
   * @return LinearColor& Reference to this
   */
  FORCEINLINE LinearColor&
  operator*=(float scalar);

  /**
   * @brief Converts to 8-bit per channel Color
   *
   * @return Color The converted color (0-255 range)
   */
  NODISCARD Color
  toColor() const;

  /**
   * @brief Clamps the color components to the [0,1] range
   *
   * @return LinearColor& Reference to this
   */
  FORCEINLINE LinearColor&
  clamp();

  /**
   * @brief Gets a clamped copy of this color
   *
   * @return LinearColor A copy with all components clamped to [0,1]
   */
  NODISCARD FORCEINLINE LinearColor
  getClamped() const;

  /**
   * @brief Component-wise access
   *
   * @param index Index (0-3 for r,g,b,a)
   * @return float& Reference to the component
   */
  NODISCARD FORCEINLINE float&
  operator[](int32 index);

  /**
   * @brief Component-wise access (const version)
   *
   * @param index Index (0-3 for r,g,b,a)
   * @return const float& Reference to the component
   */
  NODISCARD FORCEINLINE const float&
  operator[](int32 index) const;

  /**
   * @brief Get a desaturated version of this color
   *
   * @param desaturation Amount to desaturate [0,1] where 1 is fully grayscale
   * @return LinearColor The desaturated color
   */
  NODISCARD FORCEINLINE LinearColor
  desaturate(float desaturation) const;

  /**
   * @brief Compute the luminance of this color (grayscale value)
   * Uses perceptual weights for RGB: 0.3R + 0.59G + 0.11B
   *
   * @return float The luminance value in [0,1] range
   */
  NODISCARD FORCEINLINE float
  getLuminance() const;

 public:
  /**
   * @brief Creates a random color with full alpha
   *
   * @return LinearColor The randomly generated color
   */
  static LinearColor
  makeRandomColor();

  /**
   * @brief Linearly interpolate between two colors
   *
   * @param a First color
   * @param b Second color
   * @param t Interpolation factor [0,1]
   * @return LinearColor The interpolated color
   */
  NODISCARD static FORCEINLINE LinearColor
  lerp(const LinearColor& a, const LinearColor& b, float t);

  /**
   * @brief HSV to RGB conversion
   *
   * @param h Hue [0,360)
   * @param s Saturation [0,1]
   * @param v Value [0,1]
   * @param a Alpha [0,1]
   * @return LinearColor The resulting RGB color
   */
  NODISCARD static LinearColor
  fromHSV(float h, float s, float v, float a = 1.0f);

 public:
  // Predefined colors
  static const LinearColor Transparent;
  static const LinearColor White;
  static const LinearColor Gray;
  static const LinearColor Black;
  static const LinearColor Red;
  static const LinearColor Green;
  static const LinearColor Blue;
  static const LinearColor Yellow;
  static const LinearColor Cyan;
  static const LinearColor Magenta;
  static const LinearColor Light_Gray;
  static const LinearColor Dark_Gray;
  static const LinearColor Pink;
  static const LinearColor Purple;
  static const LinearColor Light_Blue;
  static const LinearColor Yellow_Green;
  static const LinearColor Orange;
  static const LinearColor Brown;
  static const LinearColor Pale_Pink;

 public:
  // Color components
  float r;
  float g;
  float b;
  float a;
};

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE
LinearColor::LinearColor() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}

/*
 */
FORCEINLINE
LinearColor::LinearColor(float _r, float _g, float _b, float _a)
 : r(_r), g(_g), b(_b), a(_a) {}

/*
 */
FORCEINLINE bool
LinearColor::equals(const LinearColor& linearColor, float tolerance) const {
  return (Math::abs(r - linearColor.r) <= tolerance) &&
         (Math::abs(g - linearColor.g) <= tolerance) &&
         (Math::abs(b - linearColor.b) <= tolerance) &&
         (Math::abs(a - linearColor.a) <= tolerance);
}

/*
 */
FORCEINLINE bool
LinearColor::operator==(const LinearColor& linearColor) const {
  return equals(linearColor, 0.0f);
}

/*
 */
FORCEINLINE bool
LinearColor::operator!=(const LinearColor& linearColor) const {
  return !equals(linearColor, 0.0f);
}

/*
 */
FORCEINLINE LinearColor&
LinearColor::operator/=(const LinearColor& linearColor) {
  r /= linearColor.r;
  g /= linearColor.g;
  b /= linearColor.b;
  a /= linearColor.a;
  return *this;
}

/*
 */
FORCEINLINE LinearColor&
LinearColor::operator*=(const LinearColor& linearColor) {
  r *= linearColor.r;
  g *= linearColor.g;
  b *= linearColor.b;
  a *= linearColor.a;
  return *this;
}

/*
 */
FORCEINLINE LinearColor&
LinearColor::operator-=(const LinearColor& linearColor) {
  r -= linearColor.r;
  g -= linearColor.g;
  b -= linearColor.b;
  a -= linearColor.a;
  return *this;
}

/*
 */
FORCEINLINE LinearColor&
LinearColor::operator+=(const LinearColor& linearColor) {
  r += linearColor.r;
  g += linearColor.g;
  b += linearColor.b;
  a += linearColor.a;
  return *this;
}

/*
 */
FORCEINLINE LinearColor
LinearColor::operator+(const LinearColor& linearColor) const {
  return LinearColor(r + linearColor.r, g + linearColor.g, b + linearColor.b,
                     a + linearColor.a);
}

/*
 */
FORCEINLINE LinearColor
LinearColor::operator-(const LinearColor& linearColor) const {
  return LinearColor(r - linearColor.r, g - linearColor.g, b - linearColor.b,
                     a - linearColor.a);
}

/*
 */
FORCEINLINE LinearColor
LinearColor::operator*(const LinearColor& linearColor) const {
  return LinearColor(r * linearColor.r, g * linearColor.g, b * linearColor.b,
                     a * linearColor.a);
}

/*
 */
FORCEINLINE LinearColor
LinearColor::operator/(const LinearColor& linearColor) const {
  return LinearColor(r / linearColor.r, g / linearColor.g, b / linearColor.b,
                     a / linearColor.a);
}

/*
 */
FORCEINLINE LinearColor
LinearColor::operator*(float scalar) const {
  return LinearColor(r * scalar, g * scalar, b * scalar, a * scalar);
}

/*
 */
FORCEINLINE LinearColor&
LinearColor::operator*=(float scalar) {
  r *= scalar;
  g *= scalar;
  b *= scalar;
  a *= scalar;
  return *this;
}

/*
 */
FORCEINLINE LinearColor&
LinearColor::clamp() {
  r = Math::clamp(r, 0.0f, 1.0f);
  g = Math::clamp(g, 0.0f, 1.0f);
  b = Math::clamp(b, 0.0f, 1.0f);
  a = Math::clamp(a, 0.0f, 1.0f);
  return *this;
}

/*
 */
FORCEINLINE LinearColor
LinearColor::getClamped() const {
  return LinearColor(Math::clamp(r, 0.0f, 1.0f), Math::clamp(g, 0.0f, 1.0f),
                     Math::clamp(b, 0.0f, 1.0f), Math::clamp(a, 0.0f, 1.0f));
}

/*
 */
FORCEINLINE float&
LinearColor::operator[](int32 index) {
  CH_ASSERT(index >= 0 && index < 4);
  return (&r)[index];
}

/*
 */
FORCEINLINE const float&
LinearColor::operator[](int32 index) const {
  CH_ASSERT(index >= 0 && index < 4);
  return (&r)[index];
}

/*
 */
FORCEINLINE LinearColor
LinearColor::desaturate(float desaturation) const {
  const float lum = getLuminance();
  return LinearColor(Math::lerp(r, lum, desaturation), Math::lerp(g, lum, desaturation),
                     Math::lerp(b, lum, desaturation), a);
}

/*
 */
FORCEINLINE float
LinearColor::getLuminance() const {
  // Perceptual luminance formula (Rec. 709 coefficients)
  return r * 0.3f + g * 0.59f + b * 0.11f;
}

/*
 */
FORCEINLINE LinearColor
LinearColor::lerp(const LinearColor& a, const LinearColor& b, float t) {
  const float invT = 1.0f - t;
  return LinearColor(a.r * invT + b.r * t, a.g * invT + b.g * t, a.b * invT + b.b * t,
                     a.a * invT + b.a * t);
}

} // namespace chEngineSDK
