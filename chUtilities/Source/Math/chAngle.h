/************************************************************************/
/**
 * @file chAngle.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 * @brief Angles in radians and in degrees.
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
 * Holds an angle in radians, so a function that takes an angle says its unit in the type.
 *
 * Radian and Degree share this header because each converts into the other inline.
 * Operators only take the same unit; convert the other one explicitly first
 * (radian + Radian(degree)).
 */
class Radian
{
 public:
  /**
   * Leaves the value uninitialized. Use Radian{} when it must start at zero.
   */
  Radian() = default;

  FORCEINLINE explicit constexpr
  Radian(float radians) noexcept;

  FORCEINLINE explicit constexpr
  Radian(const Degree& degree) noexcept;

  NODISCARD FORCEINLINE constexpr float
  valueRadian() const noexcept;

  NODISCARD FORCEINLINE constexpr float
  valueDegree() const noexcept;

  /**
   * Wraps the angle into [-PI, PI].
   */
  FORCEINLINE void
  unwind() noexcept;

  NODISCARD FORCEINLINE float
  unwindedValue() const noexcept;

  NODISCARD constexpr bool
  operator==(const Radian& other) const noexcept = default;

  NODISCARD FORCEINLINE constexpr bool
  operator<(const Radian& other) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator>(const Radian& other) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator<=(const Radian& other) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator>=(const Radian& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Radian
  operator+(const Radian& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Radian
  operator-(const Radian& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Radian
  operator-() const noexcept;

  NODISCARD FORCEINLINE constexpr Radian
  operator*(float scalar) const noexcept;

  NODISCARD FORCEINLINE constexpr Radian
  operator/(float scalar) const noexcept;

  FORCEINLINE constexpr Radian&
  operator+=(const Radian& other) noexcept;

  FORCEINLINE constexpr Radian&
  operator-=(const Radian& other) noexcept;

  FORCEINLINE constexpr Radian&
  operator*=(float scalar) noexcept;

  FORCEINLINE constexpr Radian&
  operator/=(float scalar) noexcept;

 private:
  float m_radian;
};

/**
 * Holds an angle in degrees, so a function that takes an angle says its unit in the type.
 */
class Degree
{
 public:
  /**
   * Leaves the value uninitialized. Use Degree{} when it must start at zero.
   */
  Degree() = default;

  FORCEINLINE explicit constexpr
  Degree(float degrees) noexcept;

  FORCEINLINE explicit constexpr
  Degree(const Radian& radian) noexcept;

  NODISCARD FORCEINLINE constexpr float
  valueDegree() const noexcept;

  NODISCARD FORCEINLINE constexpr float
  valueRadian() const noexcept;

  /**
   * Wraps the angle into [-180, 180].
   */
  FORCEINLINE void
  unwind() noexcept;

  NODISCARD FORCEINLINE float
  unwindedValue() const noexcept;

  NODISCARD constexpr bool
  operator==(const Degree& other) const noexcept = default;

  NODISCARD FORCEINLINE constexpr bool
  operator<(const Degree& other) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator>(const Degree& other) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator<=(const Degree& other) const noexcept;

  NODISCARD FORCEINLINE constexpr bool
  operator>=(const Degree& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Degree
  operator+(const Degree& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Degree
  operator-(const Degree& other) const noexcept;

  NODISCARD FORCEINLINE constexpr Degree
  operator-() const noexcept;

  NODISCARD FORCEINLINE constexpr Degree
  operator*(float scalar) const noexcept;

  NODISCARD FORCEINLINE constexpr Degree
  operator/(float scalar) const noexcept;

  FORCEINLINE constexpr Degree&
  operator+=(const Degree& other) noexcept;

  FORCEINLINE constexpr Degree&
  operator-=(const Degree& other) noexcept;

  FORCEINLINE constexpr Degree&
  operator*=(float scalar) noexcept;

  FORCEINLINE constexpr Degree&
  operator/=(float scalar) noexcept;

 private:
  float m_degree;
};

static_assert(std::is_trivially_copyable_v<Radian>);
static_assert(std::is_trivially_copyable_v<Degree>);
static_assert(sizeof(Radian) == 4);
static_assert(sizeof(Degree) == 4);

/************************************************************************/
/*
 * Radian implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Radian::Radian(float radians) noexcept
 : m_radian(radians)
{}

/*
 */
FORCEINLINE constexpr
Radian::Radian(const Degree& degree) noexcept
 : m_radian(degree.valueRadian())
{}

/*
 */
FORCEINLINE constexpr float
Radian::valueRadian() const noexcept
{
  return m_radian;
}

/*
 */
FORCEINLINE constexpr float
Radian::valueDegree() const noexcept
{
  return m_radian * Math::RAD2DEG;
}

/*
 */
FORCEINLINE void
Radian::unwind() noexcept
{
  m_radian = Math::unwindRadians(m_radian);
}

/*
 */
FORCEINLINE float
Radian::unwindedValue() const noexcept
{
  return Math::unwindRadians(m_radian);
}

/*
 */
FORCEINLINE constexpr bool
Radian::operator<(const Radian& other) const noexcept
{
  return m_radian < other.m_radian;
}

/*
 */
FORCEINLINE constexpr bool
Radian::operator>(const Radian& other) const noexcept
{
  return m_radian > other.m_radian;
}

/*
 */
FORCEINLINE constexpr bool
Radian::operator<=(const Radian& other) const noexcept
{
  return m_radian <= other.m_radian;
}

/*
 */
FORCEINLINE constexpr bool
Radian::operator>=(const Radian& other) const noexcept
{
  return m_radian >= other.m_radian;
}

/*
 */
FORCEINLINE constexpr Radian
Radian::operator+(const Radian& other) const noexcept
{
  return Radian(m_radian + other.m_radian);
}

/*
 */
FORCEINLINE constexpr Radian
Radian::operator-(const Radian& other) const noexcept
{
  return Radian(m_radian - other.m_radian);
}

/*
 */
FORCEINLINE constexpr Radian
Radian::operator-() const noexcept
{
  return Radian(-m_radian);
}

/*
 */
FORCEINLINE constexpr Radian
Radian::operator*(float scalar) const noexcept
{
  return Radian(m_radian * scalar);
}

/*
 */
FORCEINLINE constexpr Radian
Radian::operator/(float scalar) const noexcept
{
  return Radian(m_radian / scalar);
}

/*
 */
FORCEINLINE constexpr Radian&
Radian::operator+=(const Radian& other) noexcept
{
  m_radian += other.m_radian;
  return *this;
}

/*
 */
FORCEINLINE constexpr Radian&
Radian::operator-=(const Radian& other) noexcept
{
  m_radian -= other.m_radian;
  return *this;
}

/*
 */
FORCEINLINE constexpr Radian&
Radian::operator*=(float scalar) noexcept
{
  m_radian *= scalar;
  return *this;
}

/*
 */
FORCEINLINE constexpr Radian&
Radian::operator/=(float scalar) noexcept
{
  m_radian /= scalar;
  return *this;
}

/*
 */
NODISCARD FORCEINLINE constexpr Radian
operator*(float scalar, const Radian& radian) noexcept
{
  return radian * scalar;
}

/************************************************************************/
/*
 * Degree implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE constexpr
Degree::Degree(float degrees) noexcept
 : m_degree(degrees)
{}

/*
 */
FORCEINLINE constexpr
Degree::Degree(const Radian& radian) noexcept
 : m_degree(radian.valueDegree())
{}

/*
 */
FORCEINLINE constexpr float
Degree::valueDegree() const noexcept
{
  return m_degree;
}

/*
 */
FORCEINLINE constexpr float
Degree::valueRadian() const noexcept
{
  return m_degree * Math::DEG2RAD;
}

/*
 */
FORCEINLINE void
Degree::unwind() noexcept
{
  m_degree = Math::unwindDegrees(m_degree);
}

/*
 */
FORCEINLINE float
Degree::unwindedValue() const noexcept
{
  return Math::unwindDegrees(m_degree);
}

/*
 */
FORCEINLINE constexpr bool
Degree::operator<(const Degree& other) const noexcept
{
  return m_degree < other.m_degree;
}

/*
 */
FORCEINLINE constexpr bool
Degree::operator>(const Degree& other) const noexcept
{
  return m_degree > other.m_degree;
}

/*
 */
FORCEINLINE constexpr bool
Degree::operator<=(const Degree& other) const noexcept
{
  return m_degree <= other.m_degree;
}

/*
 */
FORCEINLINE constexpr bool
Degree::operator>=(const Degree& other) const noexcept
{
  return m_degree >= other.m_degree;
}

/*
 */
FORCEINLINE constexpr Degree
Degree::operator+(const Degree& other) const noexcept
{
  return Degree(m_degree + other.m_degree);
}

/*
 */
FORCEINLINE constexpr Degree
Degree::operator-(const Degree& other) const noexcept
{
  return Degree(m_degree - other.m_degree);
}

/*
 */
FORCEINLINE constexpr Degree
Degree::operator-() const noexcept
{
  return Degree(-m_degree);
}

/*
 */
FORCEINLINE constexpr Degree
Degree::operator*(float scalar) const noexcept
{
  return Degree(m_degree * scalar);
}

/*
 */
FORCEINLINE constexpr Degree
Degree::operator/(float scalar) const noexcept
{
  return Degree(m_degree / scalar);
}

/*
 */
FORCEINLINE constexpr Degree&
Degree::operator+=(const Degree& other) noexcept
{
  m_degree += other.m_degree;
  return *this;
}

/*
 */
FORCEINLINE constexpr Degree&
Degree::operator-=(const Degree& other) noexcept
{
  m_degree -= other.m_degree;
  return *this;
}

/*
 */
FORCEINLINE constexpr Degree&
Degree::operator*=(float scalar) noexcept
{
  m_degree *= scalar;
  return *this;
}

/*
 */
FORCEINLINE constexpr Degree&
Degree::operator/=(float scalar) noexcept
{
  m_degree /= scalar;
  return *this;
}

/*
 */
NODISCARD FORCEINLINE constexpr Degree
operator*(float scalar, const Degree& degree) noexcept
{
  return degree * scalar;
}
} // namespace chEngineSDK
