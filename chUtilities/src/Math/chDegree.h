/************************************************************************/
/**
 * @file chDegree.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 * Wrapper class for a Degree angle.
 *
 * Degree values are interchangeable with Radian values, and conversions will be done
 * automatically.
 *
 * @bug No bug known.
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
 *  Wrapper class for a Degree angle.
 *
 * Degree values are interchangeable with Radian values, and conversions will be done
 * automatically.
 *
 * Sample usage:
 *    Degree myDegree(90)
 *    float radianValue = myDegree.valueRadian();
 */
class CH_UTILITY_EXPORT Degree
{
public:
  /*
   * Default constructor
   */
  Degree() = default;

  /**
   * Constructor as default value
   */
  Degree(const Degree&) = default;

  /**
   * Default constructor from a float as degree
   *
   * @param degree
   *   Degree angle
   */
  explicit Degree(float _degree) : m_degree(_degree) {}

  /**
   * Radian constructor
   *
   * @param radian
   *   Radian to convert.
   */
  explicit Degree(const Radian& radian);

  /*
   * Default destructor
   */
  ~Degree() = default;

  /**
   * Converts the radian value to degree value.
   *
   * @param radian
   *   Radian to assign.
   *
   * @return
   *   This Degree with the new given Radian.
   */
  Degree&
  operator=(const Radian& radian);

  /**
   * Equals operator to a Degree angle value.
   *
   * @param degree
   *   Degree value
   *
   * @return
   *   This Degree with the new given Degree as float.
   */
  FORCEINLINE Degree&
  operator=(float degree);

  /**
   * Returns this angle as Radian value.
   *
   * @return
   *   Float raw value as Radian
   */
  float
  valueRadian() const;

  /**
   * Returns this angle as Degree value.
   *
   * @param degree
   *   Float value as Degree
   *
   * @return
   *   Float raw value as degree.
   */
  FORCEINLINE float
  valueDegree() const;

  /**
   * Ensures that this angle is between +/- 180.
   */
  void
  unwind();

  /**
   * Returns the unwind angle value between +/- 180.
   *
   * @return
   *   Float degree value between +/- 180
   */
  float
  unwindedValue() const;

  /************************************************************************/
  /*
   * Operator overloads.
   */
   /************************************************************************/

  /**
   * Sum operator to a Degree.
   *
   * @param degree
   *   Degree to be added.
   *
   * @return
   *   Degree created from this action.
   */
  FORCEINLINE Degree
  operator+(const Degree& degree) const;

  /**
   * Sum operator to a Radian value.
   *
   * @param radian
   *   Radian value to be added.
   *
   * @return
   *   Degree created from this action.
   */
  Degree
  operator+(const Radian& radian) const;

  /**
   * Addition assignment operator.
   *
   * @param degree
   *   Degree to be added to this.
   *
   * @return
   *   Modify this Degree depending on the math action.
   */
  FORCEINLINE Degree&
  operator+=(const Degree& degree);

  /**
   * Addition assignment operator.
   *
   * @param radian
   *   Radian to be added to this.
   *
   * @return
   *   Modify this Degree depending on the math action.
   */
  Degree&
  operator+=(const Radian& radian);

  /**
   *   Multiplies this degree by a scalar.
   *
   * @param scalar
   * The scalar to be multiplied.
   *
   * @return Degree
   *  New Degree created from the multiplication;
   **/
  FORCEINLINE Degree
  operator*(float scalar) const;

  /**
   *   Multiplies by a scalar and saves the result in this object.
   *
   * @param scalar
   * The scalar to be multiplied.
   **/
  FORCEINLINE Degree&
  operator*=(float scalar);

  /**
   * Creates a new negative Degree with the  given Degree.
   *
   * @return
   *   Degree created from this action.
   */
  FORCEINLINE Degree
  operator-() const;

  /**
   * Creates a mew Degree with the result of lDegree - rDegree.
   *
   * @param degree
   *   Degree to be subtracted.
   *
   * @return
   *   Degree created from this action.
   */
  FORCEINLINE Degree
  operator-(const Degree& degree) const;

  /**
   * Creates a new DEgree with the result of lDegree - rRadian.
   *
   * @param degree
   *   Radian to be subtracted.
   *
   * @return
   *   Degree created from this action.
   */
  Degree
  operator-(const Radian& degree) const;

  /**
   * Subtract assignment operator.
   *
   * @param degree
   *   Degree to be subtracted from this.
   *
   * @return
   *   Modify this Degree depending on the math action.
   */
  FORCEINLINE Degree&
  operator-=(const Degree& degree);

  /**
   * Subtract assignment operator.
   *
   * @param radian
   *   Radian to be subtracted.
   *
   * @return
   *    Modify this Degree depending on the math action.
   */
  Degree&
  operator-=(const Radian& radian);

  /**
   * Less than operator overload.
   *
   * @param degree
   *    Degree to compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator<(const Degree& degree) const;

  /**
   * Greater than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator>(const Degree& degree) const;

  /**
   * Less-equals than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator<=(const Degree& degree) const;

  /**
   * Greater-equals than operator overload.
   *
   * @param
   *   Degree Degree to compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator>=(const Degree& degree) const;

  /**
   * Equals than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator==(const Degree& degree) const;

  /**
   * Not equals than operator overload.
   *
   * @param degree
   *   Degree to compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator!=(const Degree& degree) const;

  /**
   * Less than operator overload.
   *
   * @param degree
   *   Float value as Degree to be compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator<(float degree) const;

  /**
   * Greater than operator overload.
   *
   * @param degree
   *   Float value as Degree to be compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator>(float degree) const;

  /**
   * Less-equals than operator overload.
   *
   * @param degree
   *   Float value as Degree to be compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator<=(float degree) const;

  /**
   * Greater-equals than operator overload.
   *
   * @param degree
   *   Float value as Degree to be compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator>=(float degree) const;

  /**
   * Equals than operator overload.
   *
   * @param degree
   *   Float value as Degree to be compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator==(float degree) const;

  /**
   * Not equals than operator overload.
   *
   * @param degree
   *   Float value as Degree to be compare.
   *
   * @return
   *   Bool from condition.
   */
  FORCEINLINE bool
  operator!=(float degree) const;

  /**
   * Less than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Bool from condition.
   */
  bool
  operator<(const Radian& radian) const;

  /**
   * Greater than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Bool from condition.
   */
  bool
  operator>(const Radian& radian) const;

  /**
   * Less-equals than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Bool from condition.
   */
  bool
  operator<=(const Radian& radian) const;

  /**
   * Greater-equals than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Bool from condition.
   */
  bool
  operator>=(const Radian& radian) const;

  /**
   * Equals than operator overload.
   *
   * @param radian
   *   Radian to compare.
   *
   * @return
   *   Bool from condition.
   */
  bool
  operator==(const Radian& radian) const;

  /**
   * Not equals than operator overload.
   *
   * @param radian
   *    Radian to compare.
   *
   * @return
   *   Bool from condition.
   */
  bool
  operator!=(const Radian& radian) const;


  /************************************************************************/
  /*
   * Friend methods for float overloads.
   */
   /************************************************************************/
private:
  /**
   * Less than operator overload.
   *
   * @param degree
   *    float value as Degree to be compare.
   *
   * @param degree
   *   Degree value to be compare.
   *
   * @return
   *   Compares a float against given Degree.
   */
  FORCEINLINE friend bool
  operator<(float floatDegree, const Degree& degree);

  /**
   * Greater than operator overload.
   *
   * @param degree
   *   float value as Degree to be compare.
   *
   * @param degree
   *   Degree value to be compare.
   *
   * @return
   *   Compares a float against given Degree.
   */
  FORCEINLINE friend bool
  operator>(float floatDegree, const Degree& degree);

  /**
   * Less-equals than operator overload.
   *
   * @param degree
   *    Float value as Degree to be compare.
   *
   * @param degree
   *    Degree value to be compare.
   *
   * @return
   *   Compares a float against given Degree.
   */
  FORCEINLINE friend bool
  operator<=(float floatDegree, const Degree& degree);

  /**
   * Greater-equals than operator overload.
   *
   * @param degree
   *   Float value as Degree to be compare.
   *
   * @param degree
   *   Degree value to be compare.
   *
   * @return
   *   Compares a float against given Degree.
   */
  FORCEINLINE friend bool
  operator>=(float floatDegree, const Degree& degree);

  /**
   * Equals than operator overload.
   *
   * @param degree
   *    Float value as Degree to be compare.
   *
   * @param degree
   *   Degree value to be compare.
   *
   * @return
   *   Compares a float against given Degree.
   */
  FORCEINLINE friend bool
  operator==(float floatDegree, const Degree& degree);

  /**
   * Not equals than operator overload.
   *
   * @param degree
   *   Float value as Degree to be compare.
   *
   * @param degree
   *   Degree value to be compare.
   *
   * @return
   *   Compares a float against given Degree.
   */
  FORCEINLINE friend bool
  operator!=(float floatDegree, const Degree& degree);

private:

  /**
   * angles in degree
   */
  float m_degree;
};

/************************************************************************/
/*
 * Implementations.
 */
 /************************************************************************/
 /*
 */
FORCEINLINE float
Degree::valueDegree() const
{
  return m_degree;
}

/*
*/
FORCEINLINE Degree&
Degree::operator=(float degree)
{
  m_degree = degree;
  return *this;
}

/*
*/
FORCEINLINE Degree
Degree::operator+(const Degree& degree) const
{
  return Degree(m_degree + degree.m_degree);
}

/*
*/
FORCEINLINE Degree&
Degree::operator+=(const Degree& degree)
{
  m_degree += degree.m_degree;
  return *this;
}

/*
*/
FORCEINLINE Degree
Degree::operator*(float scalar) const
{
  Degree d = *this;
  d *= scalar;
  return d;
}

/*
*/
FORCEINLINE Degree&
Degree::operator*=(float scalar)
{
  m_degree *= scalar;
  return *this;
}

/*
*/
FORCEINLINE Degree
Degree::operator-() const
{
  return Degree(-m_degree);
}

/*
*/
FORCEINLINE Degree
Degree::operator-(const Degree& degree) const
{
  return Degree(m_degree - degree.m_degree);
}

/*
*/
FORCEINLINE Degree&
Degree::operator-=(const Degree& degree)
{
  m_degree -= degree.m_degree;
  return *this;
}

/*
*/
FORCEINLINE bool
Degree::operator<(const Degree& degree) const
{
  return m_degree < degree.m_degree;
}

/*
*/
FORCEINLINE bool
Degree::operator>(const Degree& degree) const
{
  return m_degree > degree.m_degree;
}

/*
*/
FORCEINLINE bool
Degree::operator<=(const Degree& degree) const
{
  return m_degree <= degree.m_degree;
}

/*
*/
FORCEINLINE bool
Degree::operator>=(const Degree& degree) const
{
  return m_degree >= degree.m_degree;
}

/*
*/
FORCEINLINE bool
Degree::operator==(const Degree& degree) const
{
  return m_degree == degree.m_degree;
}

/*
*/
FORCEINLINE bool
Degree::operator!=(const Degree& degree) const
{
  return m_degree != degree.m_degree;
}

/*
*/
FORCEINLINE bool
Degree::operator<(float degree) const
{
  return m_degree < degree;
}

/*
*/
FORCEINLINE bool
Degree::operator>(float degree) const
{
  return m_degree > degree;
}

/*
*/
FORCEINLINE bool
Degree::operator<=(float degree) const
{
  return m_degree <= degree;
}

/*
*/
FORCEINLINE bool
Degree::operator>=(float degree) const
{
  return m_degree >= degree;
}

/*
*/
FORCEINLINE bool
Degree::operator==(float degree) const
{
  return m_degree == degree;
}

/*
*/
FORCEINLINE bool
Degree::operator!=(float degree) const
{
  return m_degree != degree;
}

/*
*/
FORCEINLINE  bool
operator<(float floatDegree, const Degree& degree)
{
  return floatDegree < degree.m_degree;
}

/*
*/
FORCEINLINE bool
operator>(float floatDegree, const Degree& degree)
{
  return floatDegree > degree.m_degree;
}

/*
*/
FORCEINLINE bool
operator>=(float floatDegree, const Degree& degree)
{
  return floatDegree >= degree.m_degree;
}

/*
*/
FORCEINLINE bool
operator<=(float floatDegree, const Degree& degree)
{
  return floatDegree <= degree.m_degree;
}
/*
*/
FORCEINLINE bool
operator!=(float floatDegree, const Degree& degree)
{
  return floatDegree != degree.m_degree;
}

/*
*/
FORCEINLINE bool
operator==(float floatDegree, const Degree& degree)
{
  return floatDegree == degree.m_degree;
}


}
