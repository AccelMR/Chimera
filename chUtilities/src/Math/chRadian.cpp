/************************************************************************/
/**
 * @file chRadian.cpp
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 * @brief Radian class that could be converted to degree easy.
 *
 * Radian values are interchangeable with Degree values, and conversions will be done
 * automatically
 *
 * @bug Not bug known.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
 /************************************************************************/
#include "chRadian.h"

#include "chDegree.h"
#include "chMath.h"

namespace chEngineSDK {

/*
*/
Radian::Radian(const Degree& degree) : m_radian(degree.valueRadian())
{}

/*
*/
Radian&
Radian::operator=(const Degree& degree)
{
  m_radian = degree.valueRadian();
  return *this;
}

/*
*/
float
Radian::valueDegree() const
{
  return m_radian * Math::RAD2DEG;
}

/*
*/
void
Radian::unwind()
{
  m_radian = Math::unwindRadians(m_radian);
}

/*
*/
float
Radian::unwindedValue() const
{
  return Math::unwindRadians(m_radian);
}

/*
*/
Radian
Radian::operator+(const Degree& degree) const
{
  return Radian(m_radian + degree.valueRadian());
}

/*
*/
Radian&
Radian::operator+=(const Degree& degree)
{
  m_radian += degree.valueRadian();
  return *this;
}

/*
*/
Radian
Radian::operator-(const Degree& degree) const
{
  return Radian(m_radian - degree.valueRadian());
}

/*
*/
Radian&
Radian::operator-=(const Degree& degree)
{
  m_radian -= degree.valueRadian();
  return *this;
}

/*
*/
bool
Radian::operator<(const Degree& degree) const
{
  return m_radian < degree.valueRadian();
}

/*
*/
bool
Radian::operator>(const Degree& degree) const
{
  return m_radian > degree.valueRadian();
}

/*
*/
bool
Radian::operator>=(const Degree& degree) const
{
  return m_radian >= degree.valueRadian();
}

/*
*/
bool
Radian::operator!=(const Degree& degree) const
{
  return m_radian != degree.valueRadian();
}

/*
*/
bool
Radian::operator==(const Degree& degree) const
{
  return m_radian == degree.valueRadian();
}

/*
*/
bool
Radian::operator<=(const Degree& degree) const
{
  return m_radian <= degree.valueRadian();
}
}

