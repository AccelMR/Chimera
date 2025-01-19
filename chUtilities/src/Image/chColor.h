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

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"


namespace chEngineSDK{
class CH_UTILITY_EXPORT Color
{
 public:
  /*
  *   Default constructor
  */
  Color() = default;

 /** 
  *   Constructor from 3 or 4 uint8 values.
  **/
  constexpr Color(uint8 _r, uint8 _g, uint8 _b, uint8 _a = 255) 
    : rgba{ _r, _g, _b, _a }
  {}

  /**
*   Constructor from LinearColor.
**/
  constexpr Color(const LinearColor& linearColor);

  /*
   * Convert the color to a 16-bit value
   * @param isRGB565: true if the color should be converted to RGB565, false if it should be converted to RGB555
   * @return: 16-bit value
   */
  uint16
  to16Bit(bool bIsRGB565 = false) const;

  /*
   *   Default destructor
   */
  ~Color() = default;

  /** 
   *   Returns true if color is equal to another color.
   * 
   * @param color
   *    Color to check
   **/
  FORCEINLINE bool
  equals(const Color& color) const;
  
  /** 
   *   True if both colors are exactly equal.
   **/
  FORCEINLINE bool
  operator==(const Color& color) noexcept;
  
  /** 
   *   True if both colors are different.
   **/  
  FORCEINLINE bool
  operator!=(const Color& color) const;
    
  /** 
   *   Adds two colors.
   **/
  FORCEINLINE Color
  operator+(const Color& color) const;
    
  /** 
   *   Modifies this by adding a color;
   **/
  FORCEINLINE Color&
  operator+=(const Color& color);
  
  /** 
   *   Subtracts two colors.
   **/
  FORCEINLINE Color
  operator-(const Color& color) const;
    
  /** 
   *   Modifies this by subtracting a color
   **/
  FORCEINLINE Color&
  operator-=(const Color& color);
  
  /** 
   *   Multiplies this color against another.
   **/
  FORCEINLINE Color
  operator*(const Color& color) const;
  
  /** 
   *   Modifies this by multiplying by another color.
   **/
  FORCEINLINE Color&
  operator*=(const Color& color);
   
  /** 
   *   Divides this color against another.
   **/
  FORCEINLINE Color
  operator/(const Color& color) const;
    
  /** 
   *  Modifies this by Dividing by another color.
   **/
  FORCEINLINE Color&
  operator/=(const Color& color);

 public:
  static Color
  makeRandomColor();

  /*
   * Convert a 16-bit value to a color
   * @param value: 16-bit value
   * @param isRGB565: true if the value is in RGB565 format, false if it is in RGB555 format
   * @return: color
   */
  static Color
  from16Bit(uint16 value, bool bIsRGB565 = false);


 public:
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
  union
  {
    struct
    {
      uint8 r;
      uint8 g;
      uint8 b;
      uint8 a;
    };
  
    /**
     *   Access to rgba.
     **/
    Array<uint8, 4> rgba;

    /**
     * Access to xyz.
     */
    Array<uint8, 3> rgb;

    /**
     * Access to xy.
     */
    Array<uint8, 2> rg;
  };
};

/************************************************************************/
/*
 * Implementation
 */
 /************************************************************************/
 /*
 */
FORCEINLINE bool
Color::equals(const Color& color) const {
  return  (r == color.r) &&
    (g == color.g) &&
    (b == color.b) &&
    (a == color.a);
}

/*
*/
FORCEINLINE bool
Color::operator==(const Color& color) noexcept {
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
  r /= color.r;
  g /= color.g;
  b /= color.b;
  a /= color.a;
  return *this;
}

/*
*/
FORCEINLINE Color&
Color::operator*=(const Color& color) {
  r *= color.r;
  g *= color.g;
  b *= color.b;
  a *= color.a;
  return *this;
}

/*
*/
FORCEINLINE Color&
Color::operator-=(const Color& color) {
  r -= color.r;
  g -= color.g;
  b -= color.b;
  a -= color.a;
  return *this;
}

/*
*/
FORCEINLINE Color&
Color::operator+=(const Color& color) {
  r += color.r;
  g += color.g;
  b += color.b;
  a += color.a;
  return *this;
}

/*
*/
FORCEINLINE Color
Color::operator+(const Color& color) const {
  return { static_cast<uint8>(r + color.r),
          static_cast<uint8>(g + color.g),
          static_cast<uint8>(b + color.b),
          static_cast<uint8>(a + color.a) };
}

/*
*/
FORCEINLINE Color
Color::operator-(const Color& color) const {
  return { static_cast<uint8>(r - color.r),
          static_cast<uint8>(g - color.g),
          static_cast<uint8>(b - color.b),
          static_cast<uint8>(a - color.a) };
}

/*
*/
FORCEINLINE Color
Color::operator*(const Color& color) const {
  return { static_cast<uint8>(r * color.r),
          static_cast<uint8>(g * color.g),
          static_cast<uint8>(b * color.b),
          static_cast<uint8>(a * color.a) };
}

/*
*/
FORCEINLINE Color
Color::operator/(const Color& color) const {
  return { static_cast<uint8>(r / color.r),
          static_cast<uint8>(g / color.g),
          static_cast<uint8>(b / color.b),
          static_cast<uint8>(a / color.a) };
}


}

