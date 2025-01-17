/************************************************************************/
/**
 * @file chRadian.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 *  Radian class that could be converted to degree easy.
 *
 * Radian values are interchangeable with Degree values, and conversions will be done
 * automatically
 *
 * @bug Not bug known.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {
/**
 *   Radian class that could be converted to degree easy.
 *
 * Radian values are interchangeable with Degree values, and conversions will be done
 * automatically
 *
 * Sample usage:
 *    Radian myradian(Math::PI)
 *    float degreeValue = myradian.valueDegeree();
 */
class CH_UTILITY_EXPORT Radian
{
public:
  /*
   *  Default constructor
   */
  Radian() = default;

  /**
   *  Copy constructor default.
   */
  Radian(const Radian&) = default;

  /**
   *  Explicit constructor from radian raw value.
   *
   * @param radian
   *   Float radian angle value.
   */
  explicit Radian(float _radian) : m_radian(_radian) 
  {}

  /**
   *  Constructor from degree.
   *
   * @param degree
   *   Degree to be converted to radian
   */
  explicit Radian(const Degree& degree);

  /*
   *  Default destructor
   */
  ~Radian() = default;

  /**
   *  Equals operator from Degree.
   *
   * @param degree
   *    Degree to be converted to Radians.
   *
   * @return
   *   Modify this radian with given value.
   */
  Radian&
  operator=(const Degree& degree);

  /**
   *  Equals operator from radian raw value.
   *
   * @param radian
   *   Float radian angle value.
   *
   * @return
   *   Modify this radian with given value.
   */
  FORCEINLINE Radian&
  operator=(const float& radian);


  /**
   *  Returns this angle as Degrees.
   *
   * @return
   *   This Radian as degree.
   */
  float
  valueDegree() const;

  /**
   *  Returns this angle as Radian raw value.
   *
   * @return
   *   Radian raw value.
   */
  FORCEINLINE float
  valueRadian() const;

  /**
   *  Ensures that this angle will be between +/- PI range.
   */
  void
  unwind();

  /**
   *  Returns the value unwinded, this value will be between +/- PI range.
   *
   * @return
   *   Float value unwinded
   */
  float
  unwindedValue() const;

  /************************************************************************/
  /*
   * Operator overloads.
   */
   /************************************************************************/

  /**
   *  Sum operator to a Radian value.
   *
   * @param radian
   *   Radian to added.
   *
   * @return
   *   Creates a new Radian depending on overload action.
   */
  FORCEINLINE Radian
  operator+(const Radian& radian) const;

  /**
   *  Sum operator to a Degree angle.
   *
   * @param degree
   *   Degree value to be added.
   *
   * @return
   *   Creates a new Radian depending on overload action.
   */
  Radian
  operator+(const Degree& degree) const;

  /**
   *  Additional assignment operator.
   *
   * @param radian
   *   Radian to be added to this.
   *
   * @return
   *   Modify this Radian depending on overload action.
   */
  FORCEINLINE Radian&
  operator+=(const Radian& radian);

  /**
   *  Additional assignment operator.
   *
   * @param degree
   *   Degree to be added to this.
   *
   * @return
   *    Modify this Radian depending on overload action.
   */
  Radian&
  operator+=(const Degree& degree);

  /**
   *  Creates a new negative Radian with the given Radian.
   *
   * @return
   *   Creates a new Radian depending on overload action.
   */
  FORCEINLINE Radian
  operator-() const;

  /**
   *  Creates a new Radian with the result of lRadian - rRadian.
   *
   * @param radian
   *   Radian to be subtracted
   *
   * @return
   *   Creates a new Radian depending on overload action.
   */
  FORCEINLINE Radian
  operator-(const Radian& radian) const;

  /**
   *  Creates a new Radian with result of lRadian - rDegree.
   *
   * @param degree
   *   Degree to be subtracted.
   *
   * @return
   *   Creates a new Radian depending on overload action.
   */
  Radian
  operator-(const Degree& degree) const;

  /**
   *  Subtract assignment operator.
   *
   * @param degree
   *   Degree to be subtracted.
   */
  FORCEINLINE Radian&
  operator-=(const Radian& radian);

  /**
   *  Subtract assignment operator.
   *
   * @param degree
   *   Degree to be subtracted.
   *
   * @return
   *   Modify this Radian depending on overload action.
   */
  Radian&
  operator-=(const Degree& degree);

  /**
   *  Less than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator<(const Radian& radian) const;

  /**
   *  Greater than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator>(const Radian& radian) const;

  /**
   *  Less-equals than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator<=(const Radian& radian) const;

  /**
   *  Greater-equals than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator>=(const Radian& radian) const;

  /**
   *  Equals than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator==(const Radian& radian) const;

  /**
   *  Not equals than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator!=(const Radian& radian) const;

  /**
   *  Less than operator overload.
   *
   * @param radian
   *   Float value as Radian to be compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator<(const float& radian) const;

  /**
   *  Greater than operator overload.
   *
   * @param radian
   *   Float value as Radian to be compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator>(const float& radian) const;

  /**
   *  Less-equals than operator overload.
   *
   * @param radian
   *   Float value as Radian to be compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator<=(const float& radian) const;

  /**
   *  Greater-equals than operator overload.
   *
   * @param radian
   *   Float value as Radian to be compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator>=(const float& radian) const;

  /**
   *  Equals than operator overload.
   *
   * @param radian
   *   Float value as Radian to be compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator==(const float& radian) const;

  /**
   *  Not equals than operator overload.
   *
   * @param radian
   *   Float value as Radian to be compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  FORCEINLINE bool
  operator!=(const float& radian) const;

  /**
   *  Less than operator overload.
   *
   * @param degree
   *    Degree to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  bool
  operator<(const Degree& degree) const;

  /**
   *  Greater than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  bool
  operator>(const Degree& degree) const;

  /**
   *  Less-equals than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  bool
  operator<=(const Degree& degree) const;

  /**
   *  Greater-equals than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  bool
  operator>=(const Degree& degree) const;

  /**
   *  Equals than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  bool
  operator==(const Degree& degree) const;

  /**
   *  Not equals than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Compares this Radian against given value.
   */
  bool
  operator!=(const Degree& degree) const;

  /************************************************************************/
  /*
   * Friend methods for float overloads.
   */
   /************************************************************************/
private:
  /**
   *  Less than operator overload.
   *
   * @param radian
   *   Float value as Radian to be compare.
   *
   * @param radian
   *   Radian value to be compare.
   *
 * @return
  *    Compares a float against given Radian.
   */
  FORCEINLINE friend bool
  operator<(const float& floatRadian, const Radian& radian);

  /**
   *  Greater than operator overload.
   *
   * @param radian
   *   Float value as Radian to be compare.
   *
   * @param radian
   *    Radian value to be compare.
   *
   * @return
   *   Compares a float against given Radian.
   */
  FORCEINLINE friend bool
  operator>(const float& floatRadian, const Radian& radian);

  /**
   *  Less-equals than operator overload.
   *
   * @param radian
   *   float value as Radian to be compare.
   *
   * @param radian
   *   Radian value to be compare.
   *
   * @return
   *   Compares a float against given Radian.
   */
  FORCEINLINE friend bool
  operator<=(const float& floatRadian, const Radian& radian);

  /**
   *  Greater-equals than operator overload.
   *
   * @param radian
   *    Float value as Radian to be compare.
   *
   * @param radian
   *   Radian value to be compare.
   *
   * @return Compares a float against given Radian.
   */
  FORCEINLINE friend bool
  operator>=(const float& floatRadian, const Radian& radian);

  /**
   *  Equals than operator overload.
   *
   * @param radian
   *   Float value as Radian to be compare.
   *
   * @paramradian
   *   Radian value to be compare.
   *
   * @return
   *   Compares a float against given Radian.
   */
  FORCEINLINE friend bool
  operator==(const float& floatRadian, const Radian& radian);

  /**
   *  Not equals than operator overload.
   *
   * @param radian
   *   float value as Radian to be compare.
   *
   * @param radian
   *   Radian value to be compare.
   *
   * @return
   *   Compares a float against given Radian.
   */
  FORCEINLINE friend bool
  operator!=(const float& floatRadian, const Radian& radian);


private:
  /**
   *  Angle as radian.
   */
  float m_radian;
};

/************************************************************************/
/*
 * Function implementations.
 */
 /************************************************************************/

 /*
 */
FORCEINLINE float
Radian::valueRadian() const
{
  return m_radian;
}

/*
*/
FORCEINLINE Radian&
Radian::operator=(const float& radian)
{
  m_radian = radian;
  return *this;
}

/*
 */
FORCEINLINE Radian
Radian::operator+(const Radian& radian) const
{
  return Radian(m_radian + radian.m_radian);
}

/*
*/
FORCEINLINE Radian&
Radian::operator+=(const Radian& radian)
{
  m_radian += radian.m_radian;
  return *this;
}

/*
*/
FORCEINLINE Radian
Radian::operator-() const
{
  return Radian(-m_radian);
}

/*
*/
FORCEINLINE Radian
Radian::operator-(const Radian& radian) const
{
  return Radian(m_radian - radian.m_radian);
}

/*
*/
FORCEINLINE Radian&
Radian::operator-=(const Radian& radian)
{
  m_radian -= radian.m_radian;
  return *this;
}

/*
*/
FORCEINLINE bool
Radian::operator<(const Radian& radian) const
{
  return m_radian < radian.m_radian;
}

/*
*/
FORCEINLINE bool
Radian::operator<(const float& radian) const
{
  return m_radian < radian;
}

/*
*/
FORCEINLINE bool
Radian::operator>(const Radian& radian) const
{
  return m_radian > radian.m_radian;
}

/*
*/
FORCEINLINE bool
Radian::operator>=(const Radian& radian) const
{
  return m_radian >= radian.m_radian;
}

/*
*/
FORCEINLINE bool
Radian::operator>(const float& radian) const
{
  return m_radian > radian;
}

/*
*/
FORCEINLINE bool
Radian::operator>=(const float& radian) const
{
  return m_radian >= radian;
}

/*
*/
FORCEINLINE bool
Radian::operator!=(const float& radian) const
{
  return m_radian != radian;
}

/*
*/
FORCEINLINE bool
Radian::operator==(const float& radian) const
{
  return m_radian == radian;
}

/*
*/
FORCEINLINE bool
Radian::operator<=(const float& radian) const
{
  return m_radian <= radian;
}

/*
*/
FORCEINLINE bool
Radian::operator==(const Radian& radian) const
{
  return m_radian == radian.m_radian;
}

/*
*/
FORCEINLINE bool
Radian::operator!=(const Radian& radian) const
{
  return m_radian != radian.m_radian;
}
/*
*/
FORCEINLINE bool
Radian::operator<=(const Radian& radian) const
{
  return m_radian <= radian.m_radian;
}

/*
*/
FORCEINLINE  bool
operator<(const float& floatRadian, const Radian& radian)
{
  return floatRadian < radian.m_radian;
}

/*
*/
FORCEINLINE bool
operator>(const float& floatRadian, const Radian& radian)
{
  return floatRadian > radian.m_radian;
}

/*
*/
FORCEINLINE bool
operator>=(const float& floatRadian, const Radian& radian)
{
  return floatRadian >= radian.m_radian;
}

/*
*/
FORCEINLINE bool
operator<=(const float& floatRadian, const Radian& radian)
{
  return floatRadian <= radian.m_radian;
}
/*
*/
FORCEINLINE bool
operator!=(const float& floatRadian, const Radian& radian)
{
  return floatRadian != radian.m_radian;
}

/*
*/
FORCEINLINE bool
operator==(const float& floatRadian, const Radian& radian)
{
  return floatRadian == radian.m_radian;
}

}

