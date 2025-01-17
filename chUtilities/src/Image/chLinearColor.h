/************************************************************************/
/**
 * @file chLinearColor.h
 * @author AccelMR
 * @date 2022/09/21
 *   Linear color class that holds a color as 4 bytes normalized float RGBA
 */
 /************************************************************************/

 /************************************************************************/
 /*
  * Includes
  */                                                                     
 /************************************************************************/
 #include "chPrerequisitesUtilities.h"

 #include "chMath.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Linear color class that holds a float normalized color.
 *    E.g. Red = 1.0f, 1.0f, 1.0f, 1.0f; 
 *
 * Sample usage:
 * LinearColor White = LinearColor::White;
 * LinearColor Gray = White + LinearColor(.5f, .5f, .5f, 1f);
 * 
 * float f4[4] = Gray.prt();
 *
 */
class CH_UTILITY_EXPORT LinearColor
{
 public:
  /*
  *   Default constructor
  */
  LinearColor() = default;

 /** 
  *   Constructor from 3 or 4 floats.
  **/
  LinearColor(float _r, float _g, float _b, float _a = 1)
    : rgba{_r, _g, _b, _a}
  {}
  
  /** 
  *   Constructor from Color.
  **/
  constexpr LinearColor(const Color& color);

  /*
   *   Default destructor
   */
  ~LinearColor() = default;

  /** 
   *   Returns true if color is nearly equals to a value sent.
   * 
   * @param linearColor
   *    Color to check
   * 
   * @param tolerance
   *    The tolerance to check against, Math::KINDA_SMALL_NUMBER by default
   **/
  FORCEINLINE bool
  equals(const LinearColor& linearColor, 
        float Tolerance = Math::KINDA_SMALL_NUMBER) const ;
  
  /** 
   *   True if both colors are exactly equals.
   **/
  FORCEINLINE bool
  operator==(const LinearColor& linearColor) noexcept;
  
  /** 
   *   True if both colors are different.
   **/  
  FORCEINLINE bool
  operator!=(const LinearColor& linearColor) const;
    
  /** 
   *   Adds two colors.
   **/
  FORCEINLINE LinearColor
  operator+(const LinearColor& linearColor) const;
    
  /** 
   *   Modifies this by addin a linearColor;
   **/
  FORCEINLINE LinearColor&
  operator+=(const LinearColor& linearColor);
  
  /** 
   *   Subtracts two colors.
   **/
  FORCEINLINE LinearColor
  operator-(const LinearColor& linearColor) const;
    
  /** 
   *   Modifies this by subtracting a linearColor
   **/
  FORCEINLINE LinearColor&
  operator-=(const LinearColor& linearColor);
  
  /** 
   *   Multiplies this color against another.
   **/
  FORCEINLINE LinearColor
  operator*(const LinearColor& linearColor) const;
  
  /** 
   *   Modifies this by multiplying by another color.
   **/
  FORCEINLINE LinearColor&
  operator*=(const LinearColor& linearColor);
   
  /** 
   *   Divides this color against another.
   **/
  FORCEINLINE LinearColor
  operator/(const LinearColor& linearColor) const;
    
  /** 
   *  Modifies this by Dividing by another color.
   **/
  FORCEINLINE LinearColor&
  operator/=(const LinearColor& linearColor);

 public:
  static LinearColor
  makeRandomColor();

 public:
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
  union
  {
    struct
    {
      float r;
      float g;
      float b;
      float a;
    };
  
    /**
     *   Access to xyzw.
     **/
    Array<float, 4> rgba;
  
    /**
     * Access to xyz.
     */
    Array<float, 3> rgb;
  
    /**
     * Access to xy.
     */
    Array<float, 2> rg;
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
LinearColor::equals(const LinearColor& linearColor, 
                    float tolerance/*Math::KINDA_SMALL_NUMBER*/) const{
  return  (Math::abs(r - linearColor.r) <= tolerance) && 
          (Math::abs(g - linearColor.g) <= tolerance) && 
          (Math::abs(b - linearColor.b) <= tolerance) && 
          (Math::abs(a - linearColor.a) <= tolerance);
}

/*
*/
FORCEINLINE bool
LinearColor::operator==(const LinearColor& linearColor)noexcept  {
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
  return {r + linearColor.r, 
          g + linearColor.g, 
          b + linearColor.b,
          a + linearColor.a};
}

/*
*/
FORCEINLINE LinearColor
LinearColor::operator-(const LinearColor& linearColor) const {
  return {r - linearColor.r, 
          g - linearColor.g, 
          b - linearColor.b,
          a - linearColor.a};
}

/*
*/
FORCEINLINE LinearColor
LinearColor::operator*(const LinearColor& linearColor) const {
  return {r * linearColor.r, 
          g * linearColor.g, 
          b * linearColor.b,
          a * linearColor.a};
}

/*
*/
FORCEINLINE LinearColor
LinearColor::operator/(const LinearColor& linearColor) const {
  return {r / linearColor.r, 
          g / linearColor.g, 
          b / linearColor.b,
          a / linearColor.a};
}


}