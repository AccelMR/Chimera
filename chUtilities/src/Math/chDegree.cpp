/************************************************************************/
/**
 * @file chDegree.cpp
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 * @brief Wrapper class for a Degree angle.
 *
 * Degree values are interchangeable with Radian values, and conversions will be done
 * automatically.
 *
 * @bug No bug known.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chDegree.h"

#include "chRadian.h"
#include "chMath.h"

namespace chEngineSDK {

/*
*/
Degree::Degree(const Radian& _r) : m_degree(_r.valueDegree())
{}

/*
*/
Degree&
Degree::operator=(const Radian& radian)
{
  m_degree = radian.valueDegree();
  return *this;
}

/*
*/
float
Degree::valueRadian() const
{
  return m_degree * Math::DEG2RAD;
}

/*
*/
void
Degree::unwind()
{
  m_degree = Math::unwindDegrees(m_degree);
}

/*
*/
float
Degree::unwindedValue() const
{
  return Math::unwindDegrees(m_degree);
}

/*
*/
Degree
Degree::operator+(const Radian& radian) const
{
  return Degree(m_degree + radian.valueDegree());
}

/*
*/
Degree&
Degree::operator+=(const Radian& radian)
{
  m_degree += radian.valueDegree();
  return *this;
}

/*
*/
Degree
Degree::operator-(const Radian& radian) const
{
  return Degree(m_degree - radian.valueDegree());
}


/*
*/
Degree&
Degree::operator-=(const Radian& radian)
{
  m_degree -= radian.valueDegree();
  return *this;
}

/*
*/
bool
Degree::operator<(const Radian& radian) const
{
  return m_degree < radian.valueDegree();
}

/*
*/
bool
Degree::operator>(const Radian& radian) const
{
  return m_degree > radian.valueDegree();
}

/*
*/
bool
Degree::operator>=(const Radian& radian) const
{
  return m_degree >= radian.valueDegree();
}

/*
*/
bool
Degree::operator!=(const Radian& radian) const
{
  return m_degree != radian.valueDegree();
}

/*
*/
bool
Degree::operator==(const Radian& radian) const
{
  return m_degree == radian.valueDegree();
}

/*
*/
bool
Degree::operator<=(const Radian& radian) const
{
  return m_degree <= radian.valueDegree();
}

}