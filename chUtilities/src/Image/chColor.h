/************************************************************************/
/**
 * @file chColor.h
 * @author AccelMR
 * @date 2023/03/19
 *
 *    Color class that handles a color rgba 0-255 range.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

#include "chMath.h"

namespace chEngineSDK {
/**
 * @brief A class representing an RGBA color with 8-bit per channel precision
 *
 * Each component is in the range [0, 255]
 */
class CH_UTILITY_EXPORT Color
{
 public:
  /**
   * @brief Default constructor - initializes to black with full alpha
   */
  FORCEINLINE
  Color();

  /**
   * @brief Constructor from 3 or 4 uint8 values
   *
   * @param _r Red component [0-255]
   * @param _g Green component [0-255]
   * @param _b Blue component [0-255]
   * @param _a Alpha component [0-255], defaults to 255 (opaque)
   */
  constexpr Color(uint8 _r, uint8 _g, uint8 _b, uint8 _a = 255);

  /**
   * @brief Constructor from LinearColor
   * Converts from 0.0-1.0 range to 0-255 range
   *
   * @param linearColor The floating-point color to convert
   */
  Color(const LinearColor& linearColor);

  /**
   * @brief Destructor
   */
  ~Color() = default;

  /**
   * @brief Convert the color to a 16-bit value
   *
   * @param bIsRGB565 True for RGB565 format (5:6:5), false for RGB555 format (5:5:5)
   * @return uint16 The packed 16-bit color value
   */
  NODISCARD uint16
  to16Bit(bool bIsRGB565 = false) const;

  /**
   * @brief Returns true if color is equal to another color
   *
   * @param color Color to check against
   * @return bool True if colors are exactly equal
   */
  NODISCARD FORCEINLINE bool
  equals(const Color& color) const;

  /**
   * @brief True if both colors are exactly equal
   *
   * @param color Color to compare with
   * @return bool True if colors are equal
   */
  NODISCARD FORCEINLINE bool
  operator==(const Color& color) const;

  /**
   * @brief True if both colors are different
   *
   * @param color Color to compare with
   * @return bool True if colors are different
   */
  NODISCARD FORCEINLINE bool
  operator!=(const Color& color) const;

  /**
   * @brief Adds two colors component-wise
   * Note: This may wrap around if result exceeds 255
   *
   * @param color Color to add
   * @return Color Result of addition
   */
  NODISCARD FORCEINLINE Color
  operator+(const Color& color) const;

  /**
   * @brief Adds another color to this one component-wise
   * Note: This may wrap around if result exceeds 255
   *
   * @param color Color to add
   * @return Color& Reference to this
   */
  FORCEINLINE Color&
  operator+=(const Color& color);

  /**
   * @brief Subtracts two colors component-wise
   * Note: This may wrap around if result is negative
   *
   * @param color Color to subtract
   * @return Color Result of subtraction
   */
  NODISCARD FORCEINLINE Color
  operator-(const Color& color) const;

  /**
   * @brief Subtracts another color from this one component-wise
   * Note: This may wrap around if result is negative
   *
   * @param color Color to subtract
   * @return Color& Reference to this
   */
  FORCEINLINE Color&
  operator-=(const Color& color);

  /**
   * @brief Multiplies two colors component-wise
   * Result is scaled by 255
   *
   * @param color Color to multiply with
   * @return Color Result of multiplication
   */
  NODISCARD FORCEINLINE Color
  operator*(const Color& color) const;

  /**
   * @brief Multiplies this color by another component-wise
   * Result is scaled by 255
   *
   * @param color Color to multiply with
   * @return Color& Reference to this
   */
  FORCEINLINE Color&
  operator*=(const Color& color);

  /**
   * @brief Divides two colors component-wise
   * Note: Division by zero is not checked
   *
   * @param color Color to divide by
   * @return Color Result of division
   */
  NODISCARD FORCEINLINE Color
  operator/(const Color& color) const;

  /**
   * @brief Divides this color by another component-wise
   * Note: Division by zero is not checked
   *
   * @param color Color to divide by
   * @return Color& Reference to this
   */
  FORCEINLINE Color&
  operator/=(const Color& color);

  /**
   * @brief Multiplies all components by a scalar
   * Result is clamped to 0-255 range
   *
   * @param scalar Scalar value to multiply by
   * @return Color Result of multiplication
   */
  NODISCARD FORCEINLINE Color
  operator*(float scalar) const;

  /**
   * @brief Multiplies all components by a scalar
   * Result is clamped to 0-255 range
   *
   * @param scalar Scalar value to multiply by
   * @return Color& Reference to this
   */
  FORCEINLINE Color&
  operator*=(float scalar);

  /**
   * @brief Converts to floating-point LinearColor
   *
   * @return LinearColor The converted color (0.0-1.0 range)
   */
  NODISCARD LinearColor
  toLinearColor() const;

  /**
   * @brief Component-wise access
   *
   * @param index Index (0-3 for r,g,b,a)
   * @return uint8& Reference to the component
   */
  NODISCARD FORCEINLINE uint8&
  operator[](int32 index);

  /**
   * @brief Component-wise access (const version)
   *
   * @param index Index (0-3 for r,g,b,a)
   * @return const uint8& Reference to the component
   */
  NODISCARD FORCEINLINE const uint8&
  operator[](int32 index) const;

  /**
   * @brief Get the hex string representation of this color
   *
   * @param includeAlpha Whether to include alpha in the hex string
   * @return String The hex representation (like "#RRGGBB" or "#RRGGBBAA")
   */
  NODISCARD String
  toHexString(bool includeAlpha = false) const;

  /**
   * @brief Get the luminance of this color (grayscale value)
   * Uses perceptual weights for RGB: 0.3R + 0.59G + 0.11B
   *
   * @return uint8 The luminance value in [0,255] range
   */
  NODISCARD FORCEINLINE uint8
  getLuminance() const;

 public:
  /**
   * @brief Creates a random color with full alpha
   *
   * @return Color The randomly generated color
   */
  static Color
  makeRandomColor();

  /**
   * @brief Linearly interpolate between two colors
   *
   * @param a First color
   * @param b Second color
   * @param t Interpolation factor [0,1]
   * @return Color The interpolated color
   */
  NODISCARD static FORCEINLINE Color
  lerp(const Color& a, const Color& b, float t);

  /**
   * @brief Create a color from a 16-bit value
   *
   * @param value The 16-bit color value
   * @param bIsRGB565 True if value is in RGB565 format, false for RGB555
   * @return Color The resulting color
   */
  NODISCARD static Color
  from16Bit(uint16 value, bool bIsRGB565 = false);

  /**
   * @brief Create a color from a hex string
   *
   * @param hexString The hex string (like "#FF0000" or "FF0000" for red)
   * @return Color The resulting color, or black if invalid string
   */
  NODISCARD static Color
  fromHexString(const String& hexString);

 public:
  // Predefined colors
  static const Color Transparent;
  static const Color White;
  static const Color Gray;
  static const Color Black;
  static const Color Red;
  static const Color Green;
  static const Color Blue;
  static const Color Yellow;
  static const Color Cyan;
  static const Color Magenta;
  static const Color Light_Gray;
  static const Color Dark_Gray;
  static const Color Pink;
  static const Color Purple;
  static const Color Light_Blue;
  static const Color Yellow_Green;
  static const Color Orange;
  static const Color Brown;
  static const Color Pale_Pink;

 public:
  // Color components
  uint8 r = 0;
  uint8 g = 0;
  uint8 b = 0;
  uint8 a = 255;
};

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE
Color::Color() : r(0), g(0), b(0), a(255) {}

/*
 */
constexpr Color::Color(uint8 _r, uint8 _g, uint8 _b, uint8 _a) : r(_r), g(_g), b(_b), a(_a) {}

/*
 */
FORCEINLINE bool
Color::equals(const Color& color) const {
  return r == color.r && g == color.g && b == color.b && a == color.a;
}

/*
 */
FORCEINLINE bool
Color::operator==(const Color& color) const {
  return equals(color);
}

/*
 */
FORCEINLINE bool
Color::operator!=(const Color& color) const {
  return !equals(color);
}

/*
 */
FORCEINLINE Color&
Color::operator/=(const Color& color) {
  // Check for division by zero
  r = (color.r != 0) ? r / color.r : 0;
  g = (color.g != 0) ? g / color.g : 0;
  b = (color.b != 0) ? b / color.b : 0;
  a = (color.a != 0) ? a / color.a : 0;
  return *this;
}

/*
 */
FORCEINLINE Color&
Color::operator*=(const Color& color) {
  // Multiply and scale by 255
  r = static_cast<uint8>((r * color.r) / 255);
  g = static_cast<uint8>((g * color.g) / 255);
  b = static_cast<uint8>((b * color.b) / 255);
  a = static_cast<uint8>((a * color.a) / 255);
  return *this;
}

/*
 */
FORCEINLINE Color&
Color::operator-=(const Color& color) {
  // Handle potential wrap-around for unsigned values
  r = (r >= color.r) ? r - color.r : 0;
  g = (g >= color.g) ? g - color.g : 0;
  b = (b >= color.b) ? b - color.b : 0;
  a = (a >= color.a) ? a - color.a : 0;
  return *this;
}

/*
 */
FORCEINLINE Color&
Color::operator+=(const Color& color) {
  // Handle potential overflow
  r = static_cast<uint8>(Math::min(static_cast<int>(r) + color.r, 255));
  g = static_cast<uint8>(Math::min(static_cast<int>(g) + color.g, 255));
  b = static_cast<uint8>(Math::min(static_cast<int>(b) + color.b, 255));
  a = static_cast<uint8>(Math::min(static_cast<int>(a) + color.a, 255));
  return *this;
}

/*
 */
FORCEINLINE Color
Color::operator+(const Color& color) const {
  Color result(*this);
  result += color;
  return result;
}

/*
 */
FORCEINLINE Color
Color::operator-(const Color& color) const {
  Color result(*this);
  result -= color;
  return result;
}

/*
 */
FORCEINLINE Color
Color::operator*(const Color& color) const {
  Color result(*this);
  result *= color;
  return result;
}

/*
 */
FORCEINLINE Color
Color::operator/(const Color& color) const {
  Color result(*this);
  result /= color;
  return result;
}

/*
 */
FORCEINLINE Color
Color::operator*(float scalar) const {
  return Color(static_cast<uint8>(Math::clamp(r * scalar, 0.0f, 255.0f)),
               static_cast<uint8>(Math::clamp(g * scalar, 0.0f, 255.0f)),
               static_cast<uint8>(Math::clamp(b * scalar, 0.0f, 255.0f)), a);
}

/*
 */
FORCEINLINE Color&
Color::operator*=(float scalar) {
  r = static_cast<uint8>(Math::clamp(r * scalar, 0.0f, 255.0f));
  g = static_cast<uint8>(Math::clamp(g * scalar, 0.0f, 255.0f));
  b = static_cast<uint8>(Math::clamp(b * scalar, 0.0f, 255.0f));
  return *this;
}


/*
 */
FORCEINLINE uint8&
Color::operator[](int32 index) {
  CH_ASSERT(index >= 0 && index < 4);
  return (&r)[index];
}

/*
 */
FORCEINLINE const uint8&
Color::operator[](int32 index) const {
  CH_ASSERT(index >= 0 && index < 4);
  return (&r)[index];
}

/*
 */
FORCEINLINE uint8
Color::getLuminance() const {
  // Perceptual luminance formula (Rec. 709 coefficients)
  return static_cast<uint8>(r * 0.3f + g * 0.59f + b * 0.11f);
}

/*
 */
FORCEINLINE Color
Color::lerp(const Color& a, const Color& b, float t) {
  // Clamp t to [0,1]
  t = Math::clamp(t, 0.0f, 1.0f);
  const float invT = 1.0f - t;

  return Color(
      static_cast<uint8>(a.r * invT + b.r * t), static_cast<uint8>(a.g * invT + b.g * t),
      static_cast<uint8>(a.b * invT + b.b * t), static_cast<uint8>(a.a * invT + b.a * t));
}

} // namespace chEngineSDK
