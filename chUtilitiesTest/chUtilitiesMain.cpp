/************************************************************************/
/**
 * @file chUtilities_UnitTest.cpp
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/11
 * @brief Unit test file to test every Utility functionality.
 *
 * @bug No bug known.
 */
/************************************************************************/
// #ifdef RUN_UNIT_TESTS
#include "chAlgorithm.h"
#include "chBox2D.h"
#include "chCommandLine.h"
#include "chConfigFile.h"
#include "chConsoleVariable.h"
#include "chDegree.h"
#include "chDynamicLibManager.h"
#include "chEventSystem.h"
#include "chFileStream.h"
#include "chFileSystem.h"
#include "chHash.h"
#include "chShapeOverlap.h"
#include "chLogger.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chMatrixHelpers.h"
#include "chModule.h"
#include "chPath.h"
#include "chPlane.h"
#include "chQuaternion.h"
#include "chRadian.h"
#include "chRandom.h"
#include "chRotator.h"
#include "chSphereBoxBounds.h"
#include "chStringUtils.h"
#include "chUnicode.h"
#include "chVector2.h"
#include "chVector3.h"
#include "chVector4.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace chEngineSDK;

namespace {
// Vectors do not convert into each other, and the matrix transforms return a Vector4.
Vector3
xyzOf(const Vector4& vector)
{
  return {vector.x, vector.y, vector.z};
}
} // namespace

bool isNear(float a, float b, float epsilon = 0.0001f) {
  return (std::abs(a - b) <= epsilon);
}

/************************************************************************/
/*
 * Basic type sizes.
 */
/************************************************************************/
TEST_CASE("chUtilities - BasicTypeSize") {
  REQUIRE(sizeof(unchar) == 1);
  REQUIRE(sizeof(uint8) == 1);
  REQUIRE(sizeof(uint16) == 2);
  REQUIRE(sizeof(uint32) == 4);
  REQUIRE(sizeof(uint64) == 8);

  REQUIRE(sizeof(int8) == 1);
  REQUIRE(sizeof(int16) == 2);
  REQUIRE(sizeof(int32) == 4);
  REQUIRE(sizeof(int64) == 8);

  REQUIRE(sizeof(ANSICHAR) == 1);
  REQUIRE(sizeof(float) == 4);
  REQUIRE(sizeof(double) == 8);

  REQUIRE(sizeof(ANSICHAR) == 1);
  REQUIRE(sizeof(WCHAR16) == 2);
  REQUIRE(sizeof(WCHAR32) == 4);
  REQUIRE(sizeof(UNICHAR) == 2);
  REQUIRE(sizeof(unchar) == 1);

  REQUIRE(sizeof(TYPE_OF_NULL) == 4);
  REQUIRE(sizeof(SIZE_T) == 8);
}

/************************************************************************/
/*
 * Math trigonometric Tests.  Radian and Degree Class Non-Dependent
 */
/************************************************************************/
TEST_CASE("chUtilities - PlatformMath") {
  // Static Variables
  REQUIRE(Math::PI == Approx(3.14159274f));
  REQUIRE(Math::RAD2DEG == Approx(57.295776f));
  REQUIRE(Math::DEG2RAD == Approx(0.0174532924f));
  REQUIRE(Math::TWO_PI == Approx(6.28318548f));
  REQUIRE(Math::HALF_PI == Approx(1.57079637f));
  REQUIRE(Math::QUARTER_PI == Approx(0.78539816339f));

  // Every constant must be the float nearest to the exact value.
  constexpr double kPi = 3.14159265358979323846;
  REQUIRE(Math::PI == static_cast<float>(kPi));
  REQUIRE(Math::TWO_PI == static_cast<float>(kPi * 2.0));
  REQUIRE(Math::HALF_PI == static_cast<float>(kPi / 2.0));
  REQUIRE(Math::QUARTER_PI == static_cast<float>(kPi / 4.0));
  REQUIRE(Math::INV_PI == static_cast<float>(1.0 / kPi));
  REQUIRE(Math::RAD2DEG == static_cast<float>(180.0 / kPi));
  REQUIRE(Math::DEG2RAD == static_cast<float>(kPi / 180.0));

  // Known at compile time, so other modules fold them.
  static_assert(Math::PI > 3.14f && Math::PI < 3.15f);
  static_assert(Math::abs(-2.0f) == 2.0f);
  static_assert(Math::clamp(5, 0, 3) == 3);
  static_assert(!Math::isFinite(std::numeric_limits<float>::infinity()));

  // Functions
  REQUIRE(Math::unwindDegrees(270.0f) == Approx(-90.0f));
  REQUIRE(Math::unwindDegrees(-270.0f) == Approx(90.0f));
  REQUIRE(Math::unwindDegrees(725.0f) == Approx(5.0f));
  REQUIRE(Math::unwindDegrees(-725.0f) == Approx(-5.0f));
  REQUIRE(Math::unwindDegrees(180.0f) == 180.0f);
  REQUIRE(Math::unwindDegrees(-180.0f) == -180.0f);
  REQUIRE(Math::unwindDegrees(45.0f) == 45.0f);
  REQUIRE(Math::unwindDegrees(720.0f) == 0.0f);
  REQUIRE(Math::unwindRadians(Math::TWO_PI) == 0.0f);
  REQUIRE(Math::unwindRadians(4.71239f) == Approx(-1.5707955f));
  REQUIRE(Math::unwindRadians(-4.71239f) == Approx(1.5707955f));

  // Subtracting 360 no longer changes a float this big, so a loop would never end.
  const float bigDegrees = Math::unwindDegrees(1.0e10f);
  REQUIRE(bigDegrees >= -180.0f);
  REQUIRE(bigDegrees <= 180.0f);
  const float bigRadians = Math::unwindRadians(-1.0e10f);
  REQUIRE(bigRadians >= -Math::PI);
  REQUIRE(bigRadians <= Math::PI);

  REQUIRE(Math::square(3.0f) == 9.0f);
  REQUIRE(Math::min(2.0f, 3.0f) == 2.0f);
  REQUIRE(Math::max(2.0f, 3.0f) == 3.0f);
  REQUIRE(Math::clamp(-1.0f, 0.0f, 1.0f) == 0.0f);
  REQUIRE(Math::clamp(0.5f, 0.0f, 1.0f) == 0.5f);
  REQUIRE(Math::nearEqual(1.0f, 1.0f + Math::SMALL_NUMBER * 0.5f));
  REQUIRE_FALSE(Math::nearEqual(1.0f, 1.001f));

  // abs clears the sign bit, so -0 becomes +0.
  REQUIRE_FALSE(std::signbit(Math::abs(-0.0f)));
  REQUIRE(Math::abs(-std::numeric_limits<float>::infinity()) ==
          std::numeric_limits<float>::infinity());

  REQUIRE_FALSE(Math::isFinite(std::numeric_limits<float>::infinity()));
  REQUIRE_FALSE(Math::isFinite(-std::numeric_limits<float>::infinity()));
  REQUIRE_FALSE(Math::isFinite(std::numeric_limits<float>::quiet_NaN()));
  REQUIRE(Math::isFinite(std::numeric_limits<float>::max()));
  REQUIRE(Math::isFinite(std::numeric_limits<float>::denorm_min()));

  // sinCos against the standard library over several turns, both signs.
  float maxSinError = 0.0f;
  float maxCosError = 0.0f;
  for (int32 step = -2000; step <= 2000; ++step) {
    const float angle = static_cast<float>(step) * 0.01f;
    float sinValue = 0.0f;
    float cosValue = 0.0f;
    Math::sinCos(angle, sinValue, cosValue);
    maxSinError = Math::max(maxSinError, Math::abs(sinValue - std::sin(angle)));
    maxCosError = Math::max(maxCosError, Math::abs(cosValue - std::cos(angle)));
  }
  REQUIRE(maxSinError < 1.0e-5f);
  REQUIRE(maxCosError < 1.0e-5f);

  // Values that used to overflow the integer cast must not crash, and NaN stays NaN.
  float hugeSin = 0.0f;
  float hugeCos = 0.0f;
  Math::sinCos(1.0e20f, hugeSin, hugeCos);
  REQUIRE(Math::abs(hugeSin) <= 1.0f);
  REQUIRE(Math::abs(hugeCos) <= 1.0f);
  Math::sinCos(std::numeric_limits<float>::quiet_NaN(), hugeSin, hugeCos);
  REQUIRE_FALSE(Math::isFinite(hugeSin));
  REQUIRE_FALSE(Math::isFinite(hugeCos));

  REQUIRE(Math::sqrt(25.0f) == Approx(5.0f));
  REQUIRE(Math::invSqrt(25.0f) == Approx(0.2f));
  REQUIRE(Math::pow(5.0f, 2.0f) == Approx(25.0f));
  REQUIRE(Math::pow(3.0f, 3.0f) == Approx(27.0f));
  REQUIRE(Math::abs(-3.0f) == Approx(3.0f));

  REQUIRE(Math::lerp(56.0f, 76.0f, 4.0f) == Approx(136.0f));
  REQUIRE(Math::lerp(56.0f, 76.0f, 1.0f) == Approx(76.0f));

  REQUIRE(Math::invLerp(56.0f, 76.0f, 4.0f) == Approx(-2.6f));
  REQUIRE(Math::invLerp(56.0f, 76.0f, 1.0f) == Approx(-2.75f));

  REQUIRE(Math::isFinite(1551.0f) == true);
  REQUIRE(Math::isFinite(Math::sqrt(-1.0f)) == false);

  REQUIRE(Math::fmod(545.0f, 360.0f) == Approx(185.f));
  REQUIRE(Math::fmod(7.5f, 2.1f) == Approx(1.2f));
}

/************************************************************************/
/*
 * Radian Degree Tests.
 */
/************************************************************************/
TEST_CASE("chUtilities - Degree") {
  REQUIRE(sizeof(Degree) == 4);

  Degree DegreeFromFloat(270);
  Degree DegreeFromFloatAssign;
  DegreeFromFloatAssign = 270;
  REQUIRE(DegreeFromFloatAssign.valueDegree() == 270);

  Degree CopyConst(DegreeFromFloat);
  Degree Assignment = DegreeFromFloat;
  REQUIRE(CopyConst.valueDegree() == 270);
  REQUIRE(Assignment.valueDegree() == 270);

  REQUIRE(DegreeFromFloat.valueDegree() == Approx(270.0f));
  REQUIRE(DegreeFromFloat.valueRadian() == Approx(4.71239f));

  float unwindedValue = DegreeFromFloat.unwindedValue();
  REQUIRE(unwindedValue == Approx(-90.0f));

  DegreeFromFloat.unwind();
  REQUIRE(DegreeFromFloat.valueDegree() == Approx(-90.0f));

  Degree DegreeFromDegree(DegreeFromFloat);
  REQUIRE(DegreeFromDegree.valueDegree() == Approx(-90.0f));

  Degree DegreeDefault;
  #if USING(CH_DEBUG_MODE)
    REQUIRE(DegreeDefault.valueDegree() == 0.0f);
  #else
    REQUIRE(DegreeDefault.valueDegree() != 0.0f);
  #endif

  DegreeFromFloat = 270.0f;
  REQUIRE(DegreeFromFloat.valueDegree() == Approx(270.0f));

  Degree DegreeSum = DegreeFromFloat + DegreeFromDegree;
  REQUIRE(DegreeSum.valueDegree() == Approx(180.0f));

  REQUIRE(DegreeSum.valueRadian() == Approx(Math::PI));

  Radian TestRad(Math::PI);
  Degree DegreeFromRadian(TestRad);
  Degree DegreeFromRadAssign;
  DegreeFromRadAssign = TestRad;
  REQUIRE(DegreeFromRadAssign.valueDegree() == Approx(180.0f));
  REQUIRE(DegreeFromRadian.valueDegree() == Approx(180.0f));
  REQUIRE(DegreeFromRadian.valueRadian() == Approx(Math::PI));

  Degree DegreeSumRadian = DegreeFromRadian + TestRad;
  REQUIRE(DegreeSumRadian.valueDegree() == Approx(360.0f));

  DegreeSumRadian += TestRad;
  REQUIRE(DegreeSumRadian.valueDegree() == Approx(540.0f));

  DegreeSumRadian += DegreeFromRadian;
  REQUIRE(DegreeSumRadian.valueDegree() == Approx(720.0f));

  DegreeSumRadian = -DegreeSumRadian;
  REQUIRE(DegreeSumRadian.valueDegree() == Approx(-720.0f));

  DegreeSumRadian = -DegreeSumRadian;
  REQUIRE(DegreeSumRadian.valueDegree() == Approx(720.0f));

  DegreeSumRadian = DegreeSumRadian - DegreeFromRadian;
  REQUIRE(DegreeSumRadian.valueDegree() == Approx(540.0f));

  DegreeSumRadian = DegreeSumRadian - TestRad;
  REQUIRE(DegreeSumRadian.valueDegree() == Approx(360.0f));

  Degree DegreeToCompare1(180.0f);
  Degree DegreeToCompare2(360.0f);
  Radian RadianToCompare1(Math::TWO_PI);
  float FloatToCompare1 = 360.0f;
  float FloatToCompare2 = 180.0f;

  // Degree to Degree
  REQUIRE(DegreeToCompare1 < DegreeToCompare2);
  REQUIRE_FALSE(DegreeToCompare1 > DegreeToCompare2);
  REQUIRE(DegreeToCompare1 <= DegreeToCompare1);
  REQUIRE_FALSE(DegreeToCompare1 >= DegreeToCompare2);
  REQUIRE_FALSE(DegreeToCompare1 == DegreeToCompare2);
  REQUIRE(DegreeToCompare1 != DegreeToCompare2);

  // Degree to Radian
  REQUIRE(DegreeToCompare1 < RadianToCompare1);
  REQUIRE_FALSE(DegreeToCompare1 > RadianToCompare1);
  REQUIRE(DegreeToCompare1 <= RadianToCompare1);
  REQUIRE_FALSE(DegreeToCompare1 >= RadianToCompare1);
  REQUIRE_FALSE(DegreeToCompare1 == RadianToCompare1);
  REQUIRE(DegreeToCompare1 != RadianToCompare1);

  // Degree to float
  REQUIRE(DegreeToCompare1 < FloatToCompare1);
  REQUIRE_FALSE(DegreeToCompare1 > FloatToCompare1);
  REQUIRE(DegreeToCompare1 <= FloatToCompare1);
  REQUIRE_FALSE(DegreeToCompare1 >= FloatToCompare1);
  REQUIRE_FALSE(DegreeToCompare1 == FloatToCompare1);
  REQUIRE(DegreeToCompare1 != FloatToCompare1);

  // Float as lValue to Degree
  REQUIRE(FloatToCompare2 < DegreeToCompare2);
  REQUIRE_FALSE(FloatToCompare2 > DegreeToCompare2);
  REQUIRE(FloatToCompare2 <= DegreeToCompare2);
  REQUIRE_FALSE(FloatToCompare2 >= DegreeToCompare2);
  REQUIRE_FALSE(FloatToCompare2 == DegreeToCompare2);
  REQUIRE(FloatToCompare2 != DegreeToCompare2);

  // Float as rValue to Degree
  REQUIRE(180.0f < DegreeToCompare2);
  REQUIRE_FALSE(180.0f > DegreeToCompare2);
  REQUIRE(180.0f <= DegreeToCompare2);
  REQUIRE_FALSE(180.0f >= DegreeToCompare2);
  REQUIRE_FALSE(180.0f == DegreeToCompare2);
  REQUIRE(180.0f != DegreeToCompare2);

  // Const checks
  const Radian ConstRadian(Math::PI);
  const Degree ConstDegree(180.0f);

  Degree AddRes = ConstDegree + ConstRadian;
  REQUIRE(AddRes.valueDegree() == Approx(360.0f));

  REQUIRE(ConstDegree == ConstRadian);
  REQUIRE_FALSE(ConstDegree != ConstRadian);
  REQUIRE_FALSE(ConstDegree > ConstRadian);
  REQUIRE(ConstDegree >= ConstRadian);
  REQUIRE_FALSE(ConstDegree < ConstRadian);
  REQUIRE(ConstDegree <= ConstRadian);

  REQUIRE(ConstDegree == 180.0f);
  REQUIRE_FALSE(ConstDegree != 180.0f);
  REQUIRE_FALSE(ConstDegree > 180.0f);
  REQUIRE(ConstDegree >= 180.0f);
  REQUIRE_FALSE(ConstDegree < 180.0f);
  REQUIRE(ConstDegree <= 180.0f);

  Degree MultTest(90);
  MultTest = MultTest * 2;
  REQUIRE(MultTest == 180.0f);

  MultTest *= 0.5f;
  REQUIRE(MultTest == 90.0f);
}

TEST_CASE("chUtilities - Radian") {
  REQUIRE(sizeof(Radian) == 4);

  Radian RadianFromFloat(Math::HALF_PI);

  Radian CopyConst(RadianFromFloat);
  Radian Assignment = RadianFromFloat;

  REQUIRE(CopyConst.valueRadian() == Approx(Math::HALF_PI));
  REQUIRE(Assignment.valueDegree() == Approx(90));

  Degree TestConst(180);
  Radian RadianFromDegreeCopy(TestConst);
  REQUIRE(RadianFromDegreeCopy.valueRadian() == Approx(Math::PI));

  Radian RadianFromDegreeAssign;
  RadianFromDegreeAssign = TestConst;
  REQUIRE(RadianFromDegreeAssign.valueRadian() == Approx(Math::PI));

  Radian RadianUnwind(Math::TWO_PI);
  float unwindedVal = RadianUnwind.unwindedValue();
  REQUIRE(unwindedVal == Approx(0.0f));

  RadianUnwind.unwind();
  REQUIRE(RadianUnwind.valueRadian() == Approx(0.0f));

  Radian NormalRadian(Math::PI);
  Radian RadianToadd(Math::PI);
  NormalRadian = RadianToadd + RadianToadd;
  REQUIRE(NormalRadian.valueDegree() == Approx(360.0f));

  Degree DegreeToAdd(30.0f);
  NormalRadian = NormalRadian + DegreeToAdd;
  REQUIRE(NormalRadian.valueRadian() == Approx(6.8067842f));

  NormalRadian.unwind();
  NormalRadian += RadianToadd;
  REQUIRE(NormalRadian.valueRadian() == Approx((7.0f * Math::PI) / 6.0f));

  NormalRadian += DegreeToAdd;
  REQUIRE(NormalRadian.valueDegree() == Approx(240.0f));

  Radian NegativeRadian(-NormalRadian);
  REQUIRE(NegativeRadian.valueDegree() == Approx(-240.0f));

  Radian RadianToSubtract(Math::PI);
  NormalRadian = NormalRadian - NormalRadian;
  REQUIRE(NormalRadian.valueDegree() == Approx(0.0f));

  Degree DegreeToSubtract(90.0f);
  NormalRadian = NormalRadian - DegreeToSubtract;
  REQUIRE(NormalRadian.valueRadian() == Approx(-Math::HALF_PI));

  NormalRadian -= RadianToSubtract;
  REQUIRE(NormalRadian.valueRadian() == Approx(-4.71239f));

  NormalRadian -= DegreeToSubtract;
  REQUIRE(NormalRadian.valueRadian() == Approx(-6.2831855f));

  Radian RadianToCompare1(Math::PI);
  Radian RadianToCompare2(Math::TWO_PI);
  float FloatToCompare1 = Math::TWO_PI;
  float FloatToCompare2 = Math::PI;
  Degree DegreeToCompare1(360.0f);

  // Radian to Radian
  REQUIRE(RadianToCompare1 < RadianToCompare2);
  REQUIRE_FALSE(RadianToCompare1 > RadianToCompare2);
  REQUIRE(RadianToCompare1 <= RadianToCompare1);
  REQUIRE_FALSE(RadianToCompare1 >= RadianToCompare2);
  REQUIRE_FALSE(RadianToCompare1 == RadianToCompare2);
  REQUIRE(RadianToCompare1 != RadianToCompare2);

  // Radian to Degree
  REQUIRE(RadianToCompare1 < DegreeToCompare1);
  REQUIRE_FALSE(RadianToCompare1 > DegreeToCompare1);
  REQUIRE(RadianToCompare1 <= DegreeToCompare1);
  REQUIRE_FALSE(RadianToCompare1 >= DegreeToCompare1);
  REQUIRE_FALSE(RadianToCompare1 == DegreeToCompare1);
  REQUIRE(RadianToCompare1 != DegreeToCompare1);

  // Radian to float
  REQUIRE(RadianToCompare1 < FloatToCompare1);
  REQUIRE_FALSE(RadianToCompare1 > FloatToCompare1);
  REQUIRE(RadianToCompare1 <= RadianToCompare1);
  REQUIRE_FALSE(RadianToCompare1 >= FloatToCompare1);
  REQUIRE_FALSE(RadianToCompare1 == FloatToCompare1);
  REQUIRE(RadianToCompare1 != FloatToCompare1);

  // Float as lValue to Radian
  REQUIRE(FloatToCompare2 < RadianToCompare2);
  REQUIRE_FALSE(FloatToCompare2 > RadianToCompare2);
  REQUIRE(FloatToCompare2 <= RadianToCompare2);
  REQUIRE_FALSE(FloatToCompare2 >= RadianToCompare2);
  REQUIRE_FALSE(FloatToCompare2 == RadianToCompare2);
  REQUIRE(FloatToCompare2 != RadianToCompare2);

  // Float as rValue to Radian
  REQUIRE(Math::PI < RadianToCompare2);
  REQUIRE_FALSE(Math::PI > RadianToCompare2);
  REQUIRE(Math::PI <= RadianToCompare2);
  REQUIRE_FALSE(Math::PI >= RadianToCompare2);
  REQUIRE_FALSE(Math::PI == RadianToCompare2);
  REQUIRE(Math::PI != RadianToCompare2);

  // Const checks
  const Radian ConstRadian(Math::PI);
  const Degree ConstDegree(180.0f);

  Radian AddRes = ConstRadian + ConstDegree;
  REQUIRE(AddRes.valueDegree() == Approx(360.0f));

  REQUIRE(ConstRadian == ConstDegree);
  REQUIRE_FALSE(ConstRadian != ConstDegree);
  REQUIRE_FALSE(ConstRadian > ConstDegree);
  REQUIRE(ConstRadian >= ConstDegree);
  REQUIRE_FALSE(ConstRadian < ConstDegree);
  REQUIRE(ConstRadian <= ConstDegree);

  REQUIRE(ConstRadian == Math::PI);
  REQUIRE_FALSE(ConstRadian != Math::PI);
  REQUIRE_FALSE(ConstRadian > Math::PI);
  REQUIRE(ConstRadian >= Math::PI);
  REQUIRE_FALSE(ConstRadian < Math::PI);
  REQUIRE(ConstRadian <= Math::PI);
}

/************************************************************************/
/*
 * Math trigonometric Tests.  Radian and Degree Class Dependent
 */
/************************************************************************/
TEST_CASE("chUtilities - MathTrigonometricRadianDegree") {
  Radian RadianToTest1(Math::HALF_PI);
  Radian RadianToTest2(Math::PI);
  Degree DegreeToTest1(RadianToTest1);
  Degree DegreeToTest2(RadianToTest2);

  REQUIRE(Math::cos(RadianToTest1) == Approx(0.0f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(Math::cos(DegreeToTest1) == Approx(0.0f).margin(Math::KINDA_SMALL_NUMBER));

  REQUIRE(Math::sin(RadianToTest1) == Approx(1.0f));
  REQUIRE(Math::sin(DegreeToTest1) == Approx(1.0f));

  REQUIRE(Math::tan(RadianToTest2) == Approx(0.0f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(Math::tan(DegreeToTest2) == Approx(0.0f).margin(Math::KINDA_SMALL_NUMBER));

  Radian RadiancoAcos = Math::acos(-1.0f);
  Degree DegreecoAcos;
  DegreecoAcos = Math::acos(-1.0f);
  REQUIRE(RadiancoAcos.valueDegree() == Approx(180.0f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(DegreecoAcos.valueDegree() == Approx(180.0f).margin(Math::KINDA_SMALL_NUMBER));

  Radian RadiancoAsin = Math::asin(1.0f);
  Degree DegreecoAsin;
  DegreecoAsin = Math::asin(1.0f);
  REQUIRE(RadiancoAsin.valueDegree() == Approx(90.0f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(DegreecoAsin.valueDegree() == Approx(90.0f).margin(Math::KINDA_SMALL_NUMBER));

  Radian RadiancoAtan = Math::atan(1.0f);
  Degree DegreecoAtan;
  DegreecoAtan = Math::atan(1.0f);
  REQUIRE(RadiancoAtan.valueDegree() == Approx(45.0f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(DegreecoAtan.valueDegree() == Approx(45.0f).margin(Math::KINDA_SMALL_NUMBER));

  Radian RadiancoAtan2 = Math::atan2(1.0f, 1.0f);
  Degree DegreecoAtan2(Math::atan2(1.0f, 1.0f));
  REQUIRE(RadiancoAtan2.valueRadian() ==
          Approx(0.785398163397f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(DegreecoAtan2.valueRadian() ==
          Approx(0.785398163397f).margin(Math::KINDA_SMALL_NUMBER));

  // Rounding can push a dot product of unit vectors just past 1.
  REQUIRE(Math::acos(1.0000001f).valueRadian() == 0.0f);
  REQUIRE(Math::acos(-1.0000001f).valueRadian() == Approx(Math::PI));
  REQUIRE(Math::asin(1.0000001f).valueRadian() == Approx(Math::HALF_PI));
  REQUIRE(Math::asin(-1.0000001f).valueRadian() == Approx(-Math::HALF_PI));
}

TEST_CASE("chUtilities - MathHyperbolic") {
  REQUIRE(Math::cosh(Math::PI) == Approx(11.59195328f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(Math::sinh(Math::PI) == Approx(11.548739368f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(Math::tanh(Math::HALF_PI) == Approx(0.91715234f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(Math::acosh(2.0f) == Approx(1.316957896925f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(Math::asinh(2.0f) == Approx(1.443635475179f).margin(Math::KINDA_SMALL_NUMBER));
  REQUIRE(Math::atanh(0.6f) == Approx(0.69314718056f).margin(Math::KINDA_SMALL_NUMBER));
}

/************************************************************************/
/*
 * Vectors
 */
/************************************************************************/
TEST_CASE("chUtilities - Vector2") {
  // Vector2{} value-initializes to zero; a plain declaration stays uninitialized on purpose.
  const Vector2 zeroed{};
  REQUIRE(zeroed == Vector2::ZERO);

  // The constants are constexpr.
  static_assert(Vector2::UNIT_X.dot(Vector2::UNIT_Y) == 0.0f);
  static_assert(Vector2::UNIT_X + Vector2::UNIT_Y == Vector2::UNIT);

  const float values[2] = {1.0f, 20.0f};
  REQUIRE(Vector2(values) == Vector2(1.0f, 20.0f));

  REQUIRE(Vector2::UNIT_X.cross(Vector2::UNIT_Y) == 1.0f);
  REQUIRE(Vector2::UNIT_Y.cross(Vector2::UNIT_X) == -1.0f);
  REQUIRE(Vector2(2.0f, 3.0f).dot(Vector2(4.0f, 5.0f)) == 23.0f);

  const Vector2 threeFour(3.0f, 4.0f);
  REQUIRE(threeFour.sqrMagnitude() == 25.0f);
  REQUIRE(threeFour.magnitude() == 5.0f);

  Vector2 toNormalize(15.0f, 0.0f);
  const Vector2 normalized = toNormalize.getNormalized();
  REQUIRE(toNormalize.normalize());
  REQUIRE(toNormalize == normalized);
  REQUIRE(toNormalize == Vector2::UNIT_X);
  REQUIRE(Vector2(1.0f, 1.0f).getNormalized().magnitude() == Approx(1.0f));

  // Squared length 1e-8 is under the default tolerance: normalize refuses and leaves the
  // vector as it was, getNormalized gives ZERO.
  Vector2 tooShort(1.0e-4f, 0.0f);
  REQUIRE_FALSE(tooShort.normalize());
  REQUIRE(tooShort == Vector2(1.0e-4f, 0.0f));
  REQUIRE(tooShort.getNormalized() == Vector2::ZERO);
  REQUIRE(Vector2::ZERO.getNormalized() == Vector2::ZERO);

  const Vector2 projected = Vector2(6.0f, 5.0f).projection(Vector2(10.0f, 3.0f));
  REQUIRE(projected.nearEqual(Vector2(6.88073397f, 2.06422019f)));

  REQUIRE(Vector2(1.0f, 2.0f).nearEqual(Vector2(1.0f + 1.0e-7f, 2.0f)));
  REQUIRE_FALSE(Vector2(1.0f, 2.0f).nearEqual(Vector2(1.001f, 2.0f)));

  REQUIRE(Vector2(1.0f, 2.0f) + Vector2(3.0f, 4.0f) == Vector2(4.0f, 6.0f));
  REQUIRE(Vector2(1.0f, 2.0f) - Vector2(3.0f, 4.0f) == Vector2(-2.0f, -2.0f));
  REQUIRE(-Vector2(1.0f, -2.0f) == Vector2(-1.0f, 2.0f));
  REQUIRE(Vector2(1.0f, 2.0f) * 3.0f == Vector2(3.0f, 6.0f));
  REQUIRE(3.0f * Vector2(1.0f, 2.0f) == Vector2(3.0f, 6.0f));

  Vector2 compound(1.0f, 2.0f);
  compound += Vector2(1.0f, 1.0f);
  REQUIRE(compound == Vector2(2.0f, 3.0f));
  compound -= Vector2(2.0f, 2.0f);
  REQUIRE(compound == Vector2(0.0f, 1.0f));
  compound *= 4.0f;
  REQUIRE(compound == Vector2(0.0f, 4.0f));
}

TEST_CASE("chUtilities - Vector3") {
  const Vector3 zeroed{};
  REQUIRE(zeroed == Vector3::ZERO);

  static_assert(Vector3::FORWARD.cross(Vector3::RIGHT) == Vector3::UP);
  static_assert((Vector3::UP * 2.0f).z == 2.0f);

  // World axes: X forward, Y right, Z up, left-handed.
  REQUIRE(Vector3::FORWARD == Vector3(1.0f, 0.0f, 0.0f));
  REQUIRE(Vector3::RIGHT == Vector3(0.0f, 1.0f, 0.0f));
  REQUIRE(Vector3::UP == Vector3(0.0f, 0.0f, 1.0f));
  REQUIRE(Vector3::BACKWARD == -Vector3::FORWARD);
  REQUIRE(Vector3::LEFT == -Vector3::RIGHT);
  REQUIRE(Vector3::DOWN == -Vector3::UP);
  REQUIRE(Vector3::UNIT == Vector3(1.0f, 1.0f, 1.0f));
  REQUIRE(Vector3::RIGHT.cross(Vector3::UP) == Vector3::FORWARD);
  REQUIRE(Vector3::UP.cross(Vector3::FORWARD) == Vector3::RIGHT);

  const float values[3] = {1.0f, 20.0f, 3.0f};
  REQUIRE(Vector3(values) == Vector3(1.0f, 20.0f, 3.0f));

  REQUIRE(Vector3(1.0f, 2.0f, 3.0f).dot(Vector3(4.0f, 5.0f, 6.0f)) == 32.0f);
  REQUIRE(Vector3(-1.0f, 2.0f, -3.0f).getAbs() == Vector3(1.0f, 2.0f, 3.0f));

  const Vector3 twoThreeSix(2.0f, 3.0f, 6.0f);
  REQUIRE(twoThreeSix.sqrMagnitude() == 49.0f);
  REQUIRE(twoThreeSix.magnitude() == 7.0f);
  REQUIRE(Vector3::ZERO.sqrDistance(twoThreeSix) == 49.0f);
  REQUIRE(twoThreeSix.distance(Vector3::ZERO) == 7.0f);

  Vector3 toNormalize(15.0f, 0.0f, 0.0f);
  const Vector3 normalized = toNormalize.getNormalized();
  REQUIRE(toNormalize.normalize());
  REQUIRE(toNormalize == normalized);
  REQUIRE(toNormalize == Vector3::FORWARD);
  REQUIRE(twoThreeSix.getNormalized().nearEqual(Vector3(2.0f, 3.0f, 6.0f) / 7.0f));

  Vector3 tooShort(1.0e-4f, 0.0f, 0.0f);
  REQUIRE_FALSE(tooShort.normalize());
  REQUIRE(tooShort == Vector3(1.0e-4f, 0.0f, 0.0f));
  REQUIRE(tooShort.getNormalized() == Vector3::ZERO);
  REQUIRE(Vector3::ZERO.getNormalized() == Vector3::ZERO);

  const Vector3 projected =
      Vector3(6.0f, 5.0f, 0.0f).projection(Vector3(10.0f, 3.0f, 0.0f));
  REQUIRE(projected.nearEqual(Vector3(6.88073394f, 2.06422018f, 0.0f)));

  REQUIRE(Vector3::UNIT.nearEqual(Vector3(1.0f, 1.0f + 1.0e-7f, 1.0f)));
  REQUIRE_FALSE(Vector3::UNIT.nearEqual(Vector3(1.0f, 1.0f, 1.001f)));

  REQUIRE(Vector3(1.0f, 2.0f, 3.0f) + Vector3::UNIT == Vector3(2.0f, 3.0f, 4.0f));
  REQUIRE(Vector3(1.0f, 2.0f, 3.0f) - Vector3::UNIT == Vector3(0.0f, 1.0f, 2.0f));
  REQUIRE(Vector3(1.0f, 2.0f, 3.0f) * 2.0f == Vector3(2.0f, 4.0f, 6.0f));
  REQUIRE(2.0f * Vector3(1.0f, 2.0f, 3.0f) == Vector3(2.0f, 4.0f, 6.0f));
  REQUIRE(Vector3(2.0f, 4.0f, 6.0f) / 2.0f == Vector3(1.0f, 2.0f, 3.0f));

  Vector3 compound(1.0f, 2.0f, 3.0f);
  compound += Vector3::UNIT;
  REQUIRE(compound == Vector3(2.0f, 3.0f, 4.0f));
  compound -= Vector3(2.0f, 2.0f, 2.0f);
  REQUIRE(compound == Vector3(0.0f, 1.0f, 2.0f));
  compound *= 3.0f;
  REQUIRE(compound == Vector3(0.0f, 3.0f, 6.0f));
}

TEST_CASE("chUtilities - Vector4") {
  const Vector4 zeroed{};
  REQUIRE(zeroed == Vector4::ZERO);

  static_assert(Vector4::UNIT.dot(Vector4::UNIT) == 4.0f);

  REQUIRE(Vector4::UNIT == Vector4(1.0f, 1.0f, 1.0f, 1.0f));

  const float values[4] = {1.0f, 20.0f, 0.0f, 1.0f};
  REQUIRE(Vector4(values) == Vector4(1.0f, 20.0f, 0.0f, 1.0f));

  REQUIRE(Vector4(1.0f, 2.0f, 3.0f, 4.0f).dot(Vector4(5.0f, 6.0f, 7.0f, 8.0f)) == 70.0f);
  REQUIRE(Vector4(-1.0f, 2.0f, -3.0f, -4.0f).getAbs() == Vector4(1.0f, 2.0f, 3.0f, 4.0f));

  // Cross product of xyz; w ends up 0 whatever the inputs held.
  const Vector4 crossed =
      Vector4(0.0f, 1.0f, 0.0f, 1.0f).cross(Vector4(0.0f, 0.0f, 1.0f, 1.0f));
  REQUIRE(crossed == Vector4(1.0f, 0.0f, 0.0f, 0.0f));

  // Every operation uses all four components, w included.
  const Vector4 allOnes = Vector4::UNIT;
  REQUIRE(allOnes.sqrMagnitude() == 4.0f);
  REQUIRE(allOnes.magnitude() == 2.0f);
  REQUIRE(allOnes.getNormalized() == Vector4(0.5f, 0.5f, 0.5f, 0.5f));

  Vector4 point(15.0f, 0.0f, 0.0f, 1.0f);
  REQUIRE(point.sqrMagnitude() == 226.0f);
  const Vector4 normalized = point.getNormalized();
  REQUIRE(point.normalize());
  REQUIRE(point == normalized);
  REQUIRE(point.magnitude() == Approx(1.0f));
  REQUIRE(point.w == Approx(1.0f / Math::sqrt(226.0f)));

  Vector4 tooShort(1.0e-4f, 0.0f, 0.0f, 0.0f);
  REQUIRE_FALSE(tooShort.normalize());
  REQUIRE(tooShort == Vector4(1.0e-4f, 0.0f, 0.0f, 0.0f));
  REQUIRE(tooShort.getNormalized() == Vector4::ZERO);

  REQUIRE(Vector4::UNIT.nearEqual(Vector4(1.0f, 1.0f, 1.0f, 1.0f + 1.0e-7f)));
  REQUIRE_FALSE(Vector4::UNIT.nearEqual(Vector4(1.0f, 1.0f, 1.0f, 1.001f)));

  const Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
  REQUIRE(a + Vector4::UNIT == Vector4(2.0f, 3.0f, 4.0f, 5.0f));
  REQUIRE(a - Vector4::UNIT == Vector4(0.0f, 1.0f, 2.0f, 3.0f));
  REQUIRE(-a == Vector4(-1.0f, -2.0f, -3.0f, -4.0f));
  REQUIRE(a * 2.0f == Vector4(2.0f, 4.0f, 6.0f, 8.0f));
  REQUIRE(2.0f * a == Vector4(2.0f, 4.0f, 6.0f, 8.0f));

  Vector4 compound = a;
  compound += Vector4::UNIT;
  REQUIRE(compound == Vector4(2.0f, 3.0f, 4.0f, 5.0f));
  compound -= Vector4(2.0f, 2.0f, 2.0f, 2.0f);
  REQUIRE(compound == Vector4(0.0f, 1.0f, 2.0f, 3.0f));
  compound *= 2.0f;
  REQUIRE(compound == Vector4(0.0f, 2.0f, 4.0f, 6.0f));
}

/************************************************************************/
/*
 * Rotator.
 */
/************************************************************************/
TEST_CASE("chUtilities - Rotator") {
  REQUIRE(sizeof(Rotator) == Approx(12.0f));

  Rotator ShouldTriggerWarning((float)NAN, (float)NAN, (float)NAN);
  // REQUIRE(ShouldTriggerWarning.checkIfNaN()); //Rotator fixes itself when running as debug.

  REQUIRE(Rotator::normalizeAxis(Degree(545.0f)).valueDegree() == Approx(-175.0f));
  REQUIRE(Rotator::normalizeAxis(Degree(720.0f)).valueDegree() == Approx(0.0f));

  REQUIRE(Rotator::clampAxis(Degree(540.f)).valueDegree() == Approx(180.0f));
  REQUIRE(Rotator::clampAxis(Degree(720.f)).valueDegree() == Approx(0.0f));

  Rotator NormalizeRot(720.f, 365.f, 182.f);
  Rotator NormalizedRot = NormalizeRot.getNormalized();
  REQUIRE(NormalizedRot == Rotator(0.f, 5.f, -178.f));

  NormalizeRot.normalize();
  REQUIRE(NormalizedRot == NormalizeRot);

  Rotator DenormalizeRot(720.f, 450.f, -545.f);
  Rotator DenormalizedRot = DenormalizeRot.getDenormalized();
  REQUIRE(DenormalizedRot == Rotator(0.f, 90.f, 175.f));

  DenormalizeRot.normalize();
  REQUIRE(DenormalizeRot == DenormalizeRot);
}

/************************************************************************/
/*
 * Matrix
 */
/************************************************************************/
namespace {
// Covers every branch of Quaternion(Matrix4): positive trace and each largest diagonal.
const Rotator kTestRotators[] = {
    Rotator(0.0f, 0.0f, 0.0f),      Rotator(30.0f, 0.0f, 0.0f),
    Rotator(0.0f, 30.0f, 0.0f),     Rotator(0.0f, 0.0f, 30.0f),
    Rotator(20.0f, 40.0f, 60.0f),   Rotator(0.0f, 180.0f, 0.0f),
    Rotator(0.0f, 180.0f, 180.0f),  Rotator(170.0f, 10.0f, 0.0f),
    Rotator(10.0f, 20.0f, 175.0f),  Rotator(80.0f, -120.0f, 33.0f),
    Rotator(-45.0f, 135.0f, -90.0f), Rotator(10.0f, 170.0f, 175.0f)};

bool
isSameRotation(const Matrix4& matrix, const Quaternion& quaternion)
{
  for (const Vector3& axis : {Vector3::FORWARD, Vector3::RIGHT, Vector3::UP}) {
    if (!xyzOf(matrix.transformVector(axis)).nearEqual(quaternion.rotateVector(axis), 1e-5f)) {
      return false;
    }
  }
  return true;
}

Matrix4
multiplyReference(const Matrix4& a, const Matrix4& b)
{
  Matrix4 result = Matrix4::ZERO;
  for (int32 row = 0; row < 4; ++row) {
    for (int32 column = 0; column < 4; ++column) {
      for (int32 k = 0; k < 4; ++k) {
        result[row][column] += a[row][k] * b[k][column];
      }
    }
  }
  return result;
}
} // namespace

TEST_CASE("chUtilities - Matrix4") {
  static_assert(sizeof(Matrix4) == 16 * 4);
  static_assert(alignof(Matrix4) == 16);
  static_assert(std::is_trivially_copyable_v<Matrix4>);
  static_assert(Matrix4::IDENTITY.data()[0] == 1.0f && Matrix4::IDENTITY.data()[1] == 0.0f);

  const Matrix4 Identity(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                         0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  REQUIRE(Identity == Matrix4::IDENTITY);

  Matrix4 Temporal1(Identity);
  REQUIRE(Identity == Temporal1);

  Matrix4 Temporal2(9.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
  Temporal1.at(0, 0) = 9.0f;
  REQUIRE(Temporal1 == Temporal2);

  const Matrix4 ActualResult(9.0f, 9.0f, 9.0f, 9.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                             1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
  REQUIRE(Temporal1 * Matrix4::UNITY == ActualResult);

  Temporal1 *= Matrix4::UNITY;
  REQUIRE(Temporal1 == ActualResult);

  Matrix4 AdditionResult = Temporal1 + ActualResult;
  const Matrix4 RealAdditionResult(18.0f, 18.0f, 18.0f, 18.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f,
                                   2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f);
  REQUIRE(AdditionResult == RealAdditionResult);
  REQUIRE(AdditionResult - ActualResult == ActualResult);

  const Matrix4 ValMulFixedresult(4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                  4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f);
  REQUIRE(Matrix4::IDENTITY * 4.0f == ValMulFixedresult);

  Temporal2 *= 0.0f;
  REQUIRE(Temporal2 == Matrix4::ZERO);

  Temporal1.setIdentity();
  REQUIRE(Temporal1 == Matrix4::IDENTITY);

  Temporal2 = RealAdditionResult.getTransposed();
  const Matrix4 TransposedResult(18.0f, 2.0f, 2.0f, 2.0f, 18.0f, 2.0f, 2.0f, 2.0f, 18.0f, 2.0f,
                                 2.0f, 2.0f, 18.0f, 2.0f, 2.0f, 2.0f);
  REQUIRE(Temporal2 == TransposedResult);

  Temporal2.transpose();
  REQUIRE(Temporal2 == RealAdditionResult);

  // SIMD multiplication against a plain triple loop.
  const Matrix4 A(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f,
                  13.0f, 14.0f, 15.0f, 16.0f);
  const Matrix4 B(0.5f, -1.0f, 2.0f, 0.0f, 3.0f, 0.25f, -2.0f, 1.0f, -1.5f, 4.0f, 1.0f, 2.0f,
                  0.0f, 1.0f, -3.0f, 0.75f);
  REQUIRE(A * B == multiplyReference(A, B));
  REQUIRE(B * A == multiplyReference(B, A));
  Matrix4 selfMultiplied = A;
  selfMultiplied *= selfMultiplied;
  REQUIRE(selfMultiplied == multiplyReference(A, A));

  // Determinant and inverse.
  REQUIRE(RealAdditionResult.getDeterminant() == Approx(0.0f));
  REQUIRE(RealAdditionResult.getInverse() == Matrix4::IDENTITY);

  const Matrix4 ToInverse(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 3.0f, 1.0f, 2.0f, 2.0f, 3.0f, 1.0f,
                          0.0f, 1.0f, 0.0f, 2.0f, 1.0f);
  const Matrix4 KnownInverse(-3.0f, -0.5f, 1.5f, 1.0f, 1.0f, 0.25f, -0.25f, -0.5f, 3.0f, 0.25f,
                             -1.25f, -0.5f, -3.0f, 0.0f, 1.0f, 1.0f);
  REQUIRE(ToInverse.getInverse().nearEqual(KnownInverse, 1e-5f));
  REQUIRE(ToInverse.getDeterminant() == Approx(-4.0f));
  REQUIRE((ToInverse * ToInverse.getInverse()).nearEqual(Matrix4::IDENTITY, 1e-5f));
  REQUIRE(Vector4(ToInverse.getRow(1)) == Vector4(0.0f, 3.0f, 1.0f, 2.0f));

  // Small scales used to be taken as singular and returned IDENTITY.
  for (const Vector3& scale : {Vector3(1.0f, 1.0f, 1.0f), Vector3(2.0f, 3.0f, 0.5f),
                               Vector3(0.001f, 0.002f, 0.001f), Vector3(-1.0f, 1.0f, 4.0f)}) {
    const ScaleRotationTranslationMatrix srt(scale, Rotator(20.0f, 40.0f, 60.0f),
                                             Vector3(5.0f, -2.0f, 7.0f));
    REQUIRE(srt.getDeterminant() == Approx(scale.x * scale.y * scale.z));

    const Matrix4 inverse = srt.getInverse();
    REQUIRE((inverse * srt).nearEqual(Matrix4::IDENTITY, 1e-4f));
    // With scale 0.001 the inverse moves about 5000 units, and float rounding of numbers
    // that big cancelling each other is around 1e-3.
    REQUIRE((srt * inverse).nearEqual(Matrix4::IDENTITY, 1e-2f));
    REQUIRE(srt.getInverseAffine().nearEqual(inverse, 1e-2f));
    REQUIRE((srt.getInverseAffine() * srt).nearEqual(Matrix4::IDENTITY, 1e-4f));
  }
  const ScaleRotationTranslationMatrix flat(Vector3(0.0f, 1.0f, 1.0f), Rotator::ZERO,
                                            Vector3::ZERO);
  REQUIRE(flat.getInverseAffine() == Matrix4::IDENTITY);

  // Transforms.
  const TranslationMatrix T(Vector3(10.0f, 2.0f, 1.0f));
  REQUIRE(T.transformPosition(Vector3(1.8f, 52.f, 26.6f)) == Vector4(11.8f, 54.f, 27.6f, 1.0f));
  REQUIRE(T.transformVector(Vector3(1.8f, 52.f, 26.6f)) == Vector4(1.8f, 52.f, 26.6f, 0.0f));
  REQUIRE(A.transformVector4(Vector4(1.0f, 2.0f, 3.0f, 4.0f)) ==
          Vector4(90.0f, 100.0f, 110.0f, 120.0f));

  REQUIRE(sizeof(TranslationMatrix) == 16 * 4);
  const TranslationMatrix PositionMat(Vector3(2.0f, 3.0f, 150.0f));
  REQUIRE(Vector4(PositionMat.getRow(3)) == Vector4(2.0f, 3.0f, 150.0f, 1.0f));

  // X forward, Y right, Z up: positive pitch turns forward up, positive yaw turns it right,
  // positive roll turns right down.
  const float c30 = Math::cos(Degree(30.0f));
  const float s30 = Math::sin(Degree(30.0f));
  REQUIRE(xyzOf(RotationMatrix(Rotator(30.0f, 0.0f, 0.0f)).transformVector(Vector3::FORWARD))
              .nearEqual(Vector3(c30, 0.0f, s30), 1e-6f));
  REQUIRE(xyzOf(RotationMatrix(Rotator(0.0f, 30.0f, 0.0f)).transformVector(Vector3::FORWARD))
              .nearEqual(Vector3(c30, s30, 0.0f), 1e-6f));
  REQUIRE(xyzOf(RotationMatrix(Rotator(0.0f, 0.0f, 30.0f)).transformVector(Vector3::RIGHT))
              .nearEqual(Vector3(0.0f, c30, -s30), 1e-6f));

  for (const Rotator& rotator : kTestRotators) {
    const RotationMatrix pure(rotator);
    REQUIRE((pure * pure.getTransposed()).nearEqual(Matrix4::IDENTITY, 1e-5f));
    REQUIRE(pure.getDeterminant() == Approx(1.0f));
    REQUIRE(Vector4(pure.getRow(3)) == Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    REQUIRE(RotationMatrix(pure.rotator()).nearEqual(pure, 1e-4f));
    REQUIRE(isSameRotation(pure, pure.toQuaternion()));

    const RotationTranslationMatrix moved(rotator, Vector3(1.0f, 2.0f, 3.0f));
    REQUIRE(moved == pure * TranslationMatrix(Vector3(1.0f, 2.0f, 3.0f)));
  }
  REQUIRE(RotationMatrix(Rotator(30.0f, 0.0f, 0.0f))
              .rotator()
              .nearEqual(Rotator(30.0f, 0.0f, 0.0f), 1e-3f));
  REQUIRE(RotationMatrix(Rotator(20.0f, 40.0f, 60.0f))
              .rotator()
              .nearEqual(Rotator(20.0f, 40.0f, 60.0f), 1e-3f));

  // Scale, then rotation, then translation.
  const Vector3 scale(2.0f, 3.0f, 0.5f);
  const Rotator rotation(20.0f, 40.0f, 60.0f);
  const Vector3 origin(5.0f, -2.0f, 7.0f);
  const Matrix4 scaleMatrix(scale.x, 0.0f, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, 0.0f,
                            scale.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  REQUIRE(ScaleRotationTranslationMatrix(scale, rotation, origin)
              .nearEqual(scaleMatrix * RotationMatrix(rotation) * TranslationMatrix(origin),
                         1e-5f));

  // Test PerspectiveMatrix
  Radian halfFOV(Math::PI / 4.0f); // 45 degrees
  PerspectiveMatrix perspective(halfFOV, 800.0f, 600.0f, 0.1f, 1000.0f);

  // Check key elements of the perspective matrix
  float expectedValue = 1.0f / Math::tan(halfFOV);
  REQUIRE(perspective.at(0, 0) == Approx(expectedValue));
  REQUIRE(perspective.at(1, 1) == Approx((800.0f / Math::tan(halfFOV)) / 600.0f));
  REQUIRE(perspective.at(2, 2) == Approx(1000.0f / 999.9f));
  REQUIRE(perspective.at(2, 3) == 1.0f);
  REQUIRE(perspective.at(3, 2) == Approx(-0.1f * 1000.0f / 999.9f));


  float width = 1920.0f;
  float height = 1080.0f;
  float nearPlane = 0.5f;
  float farPlane = 500.0f;
  Radian halfFOV2(Math::PI / 3.0f); // 60 degrees
  perspective = PerspectiveMatrix(halfFOV2, width, height, nearPlane, farPlane);

  float expectedX = 1.0f / Math::tan(halfFOV2);
  float expectedY = (width / Math::tan(halfFOV2)) / height;
  float expectedZ = farPlane / (farPlane - nearPlane);
  float expectedW = -(nearPlane * farPlane) / (farPlane - nearPlane);

  // Pruebas usando isNear para permitir pequeñas diferencias de precisión
  REQUIRE(isNear(perspective.at(0, 0), expectedX));
  REQUIRE(isNear(perspective.at(1, 1), expectedY));
  REQUIRE(isNear(perspective.at(2, 2), expectedZ));
  REQUIRE(isNear(perspective.at(2, 3), 1.0f));
  REQUIRE(isNear(perspective.at(3, 2), expectedW));
  REQUIRE(isNear(perspective.at(3, 3), 0.0f));

  // Asegurarse de que otros elementos que deben ser cero realmente lo sean
  REQUIRE(isNear(perspective.at(0, 1), 0.0f));
  REQUIRE(isNear(perspective.at(0, 2), 0.0f));
  REQUIRE(isNear(perspective.at(0, 3), 0.0f));
  REQUIRE(isNear(perspective.at(1, 0), 0.0f));
  REQUIRE(isNear(perspective.at(1, 2), 0.0f));
  REQUIRE(isNear(perspective.at(1, 3), 0.0f));
  REQUIRE(isNear(perspective.at(2, 0), 0.0f));
  REQUIRE(isNear(perspective.at(2, 1), 0.0f));
  REQUIRE(isNear(perspective.at(3, 0), 0.0f));
  REQUIRE(isNear(perspective.at(3, 1), 0.0f));

  // View space is X right, Y up, Z forward, like D3DXMatrixLookAtLH.
  const LookAtMatrix lookForward(Vector3::ZERO, Vector3::FORWARD * 10.0f, Vector3::UP);
  REQUIRE(lookForward.transformPosition(Vector3(10.0f, 5.0f, 0.0f)) ==
          Vector4(5.0f, 0.0f, 10.0f, 1.0f));
  REQUIRE(lookForward.transformPosition(Vector3(10.0f, 0.0f, 5.0f)) ==
          Vector4(0.0f, 5.0f, 10.0f, 1.0f));

  const Vector3 eyePos(3.0f, 2.0f, 1.0f);
  const Vector3 target(-1.0f, 4.0f, 2.0f);
  const LookAtMatrix lookAtMatrix(eyePos, target, Vector3::UP);
  REQUIRE(xyzOf(lookAtMatrix.transformPosition(eyePos)).nearEqual(Vector3::ZERO, 1e-5f));
  REQUIRE(xyzOf(lookAtMatrix.transformPosition(target))
              .nearEqual(Vector3(0.0f, 0.0f, (target - eyePos).magnitude()), 1e-5f));
  REQUIRE((lookAtMatrix * lookAtMatrix.getInverseAffine()).nearEqual(Matrix4::IDENTITY, 1e-5f));
  REQUIRE(lookAtMatrix.getDeterminant() == Approx(1.0f));

  // Looking straight up still gives a valid view.
  const LookAtMatrix lookUp(Vector3::ZERO, Vector3::UP, Vector3::UP);
  REQUIRE(lookUp.getDeterminant() == Approx(1.0f));
  REQUIRE(xyzOf(lookUp.transformPosition(Vector3::UP)).nearEqual(Vector3(0.0f, 0.0f, 1.0f),
                                                                     1e-5f));

  // Clip space is X right, Y up, depth 0 at near and 1 at far.
  const Matrix4 viewProj = lookForward * PerspectiveMatrix(Radian(Math::PI * 0.25f), 800.0f,
                                                           600.0f, 1.0f, 100.0f);
  Vector4 clip = viewProj.transformPosition(Vector3(10.0f, 5.0f, 5.0f));
  REQUIRE(clip.x / clip.w > 0.0f);
  REQUIRE(clip.y / clip.w > 0.0f);
  clip = viewProj.transformPosition(Vector3(1.0f, 0.0f, 0.0f));
  REQUIRE(clip.z / clip.w == Approx(0.0f).margin(1e-6f));
  clip = viewProj.transformPosition(Vector3(100.0f, 0.0f, 0.0f));
  REQUIRE(clip.z / clip.w == Approx(1.0f));

  const OrthographicMatrix ortho(4.0f, 2.0f, 1.0f, 11.0f);
  REQUIRE(ortho.transformPosition(Vector3(4.0f, 2.0f, 1.0f)) == Vector4(1.0f, 1.0f, 0.0f, 1.0f));
  REQUIRE(ortho.transformPosition(Vector3(-4.0f, -2.0f, 11.0f)) ==
          Vector4(-1.0f, -1.0f, 1.0f, 1.0f));
}

/************************************************************************/
/*
 * Quaternion.
 */
/************************************************************************/
TEST_CASE("chUtilities - Quaternion") {
  REQUIRE(sizeof(Quaternion) == 4 * 4);

  Quaternion quaternionDefault;
  REQUIRE(quaternionDefault.x == 0.0f);
  REQUIRE(quaternionDefault.y == 0.0f);
  REQUIRE(quaternionDefault.z == 0.0f);
  REQUIRE(quaternionDefault.w == 1.0f);
  REQUIRE(quaternionDefault == Quaternion::IDENTITY);

  // Positive pitch turns forward up, positive yaw turns it right, positive roll turns
  // right down, the same as RotationMatrix.
  const Quaternion quatPitch90(Rotator(90.0f, 0.0f, 0.0f));
  const Quaternion quatYaw90(Rotator(0.0f, 90.0f, 0.0f));
  const Quaternion quatRoll90(Rotator(0.0f, 0.0f, 90.0f));
  REQUIRE(quatPitch90.rotateVector(Vector3::FORWARD).nearEqual(Vector3::UP, 1e-5f));
  REQUIRE(quatYaw90.rotateVector(Vector3::FORWARD).nearEqual(Vector3::RIGHT, 1e-5f));
  REQUIRE(quatRoll90.rotateVector(Vector3::RIGHT).nearEqual(Vector3::DOWN, 1e-5f));
  REQUIRE(quatPitch90.nearEqual(Quaternion(0.0f, -0.707106769f, 0.0f, 0.707106769f), 1e-6f));
  REQUIRE(quatYaw90.nearEqual(Quaternion(0.0f, 0.0f, 0.707106769f, 0.707106769f), 1e-6f));
  REQUIRE(quatRoll90.nearEqual(Quaternion(-0.707106769f, 0.0f, 0.0f, 0.707106769f), 1e-6f));
  REQUIRE(Rotator(20.0f, 40.0f, 60.0f).toQuaternion() ==
          Quaternion(Rotator(20.0f, 40.0f, 60.0f)));

  REQUIRE(Quaternion(Rotator(45.0f, 45.0f, 0.0f))
              .rotateVector(Vector3::FORWARD)
              .nearEqual(Vector3(0.5f, 0.5f, 0.707106769f), 1e-5f));

  for (const Rotator& rotator : kTestRotators) {
    const RotationMatrix matrix(rotator);
    const Quaternion fromRotator(rotator);
    REQUIRE(isSameRotation(matrix, fromRotator));
    REQUIRE(isSameRotation(matrix, Quaternion(static_cast<const Matrix4&>(matrix))));

    // Euler angles have more than one answer for the same rotation, so the round trip
    // compares rotations, not angles.
    REQUIRE(RotationMatrix(fromRotator.toRotator()).nearEqual(matrix, 1e-4f));
  }

  // At pitch +-90 only yaw minus roll can be recovered, but the rotation must match.
  for (const Rotator& rotator : {Rotator(90.0f, 30.0f, 10.0f), Rotator(-90.0f, 30.0f, 10.0f)}) {
    const Rotator back = Quaternion(rotator).toRotator();
    REQUIRE(RotationMatrix(back).nearEqual(RotationMatrix(rotator), 1e-3f));
  }

  // Axis-angle follows the right-hand formula, so +90 around the right axis turns forward
  // down, which is pitch -90.
  const Quaternion axisAngleQuat(Vector3::RIGHT, Degree(90.0f));
  REQUIRE(axisAngleQuat.rotateVector(Vector3::FORWARD).nearEqual(Vector3::DOWN, 1e-5f));
  REQUIRE(axisAngleQuat.isRotationEqual(Quaternion(Rotator(-90.0f, 0.0f, 0.0f)), 1e-6f));

  Quaternion conjugated = quatYaw90;
  conjugated.conjugate();
  REQUIRE(conjugated.nearEqual(quatYaw90.getConjugated()));
  REQUIRE(conjugated.nearEqual(quatYaw90.getInverse(), 1e-6f));

  Quaternion nonUnit(1.0f, 2.0f, 3.0f, 4.0f);
  const Quaternion normalized = nonUnit.getNormalized();
  nonUnit.normalize();
  REQUIRE(nonUnit.nearEqual(normalized));
  REQUIRE(normalized.length() == Approx(1.0f));

  // Debug builds turn a NaN quaternion back into IDENTITY.
  Quaternion Qnan((float)NAN, 0.0f, 0.0f, (float)NAN);

  const Vector3 Right = quatYaw90.rotateVector(Vector3::FORWARD);
  const Vector3 Backwards = quatYaw90.rotateVector(Right);
  REQUIRE(Backwards.nearEqual(-Vector3::FORWARD, Math::KINDA_SMALL_NUMBER));

  Quaternion testQuat(1.0f, 2.0f, 3.0f, 4.0f);
  REQUIRE(testQuat[0] == 1.0f);
  REQUIRE(testQuat[1] == 2.0f);
  REQUIRE(testQuat[2] == 3.0f);
  REQUIRE(testQuat[3] == 4.0f);

  testQuat[0] = 5.0f;
  testQuat[1] = 6.0f;
  REQUIRE(testQuat.x == 5.0f);
  REQUIRE(testQuat.y == 6.0f);

  Quaternion unitQuat = Quaternion::IDENTITY;
  REQUIRE(unitQuat.squaredLength() == Approx(1.0f));
  REQUIRE(unitQuat.length() == Approx(1.0f));

  Quaternion twoQuat(2.0f, 0.0f, 0.0f, 0.0f);
  REQUIRE(twoQuat.squaredLength() == Approx(4.0f));
  REQUIRE(twoQuat.length() == Approx(2.0f));

  Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
  Quaternion q2(5.0f, 6.0f, 7.0f, 8.0f);
  Quaternion sum = q1 + q2;
  REQUIRE(sum.x == 6.0f);
  REQUIRE(sum.y == 8.0f);
  REQUIRE(sum.z == 10.0f);
  REQUIRE(sum.w == 12.0f);

  Quaternion scaled = q1 * 2.0f;
  REQUIRE(scaled.x == 2.0f);
  REQUIRE(scaled.y == 4.0f);
  REQUIRE(scaled.z == 6.0f);
  REQUIRE(scaled.w == 8.0f);

  Quaternion inPlace = q1;
  inPlace *= 2.0f;
  REQUIRE(inPlace.x == 2.0f);
  REQUIRE(inPlace.y == 4.0f);
  REQUIRE(inPlace.z == 6.0f);
  REQUIRE(inPlace.w == 8.0f);

  // A * B applies B first, then A.
  Quaternion qRoll45(Rotator(0.0f, 0.0f, 45.0f));
  Quaternion qPitch45(Rotator(45.0f, 0.0f, 0.0f));
  Quaternion combined = qRoll45 * qPitch45;
  Vector3 testVec(1.0f, 0.0f, 0.0f);
  Vector3 rotatedTwice = qRoll45.rotateVector(qPitch45.rotateVector(testVec));
  REQUIRE(rotatedTwice.nearEqual(combined.rotateVector(testVec), Math::SMALL_NUMBER));

  Quaternion arbitrary(0.1f, 0.2f, 0.3f, 0.4f);
  arbitrary.normalize();
  Quaternion shouldBeIdentity = arbitrary * arbitrary.getInverse();
  REQUIRE(shouldBeIdentity.nearEqual(Quaternion::IDENTITY, Math::SMALL_NUMBER));

  Vector3 originalVec(1.0f, 2.0f, 3.0f);
  Vector3 unrotated = arbitrary.unrotateVector(arbitrary.rotateVector(originalVec));
  REQUIRE(unrotated.nearEqual(originalVec, Math::SMALL_NUMBER));

  Quaternion fromVec4(Vector4(0.0f, 0.0f, 0.707106769f, 0.707106769f));
  REQUIRE(fromVec4.nearEqual(quatYaw90, 1e-6f));
}

/**********************************************************************/
/*
 *                            Shapes
 */
/***********************************************************************/
TEST_CASE("chUtilities - AABox") {
  REQUIRE(sizeof(AABox) == 12 * 2);

  const AABox UnitBox(Vector3::ZERO, Vector3::UNIT);
  REQUIRE(UnitBox.getCenter() == Vector3(0.5f, 0.5f, 0.5f));
  REQUIRE(UnitBox.getSize() == Vector3::UNIT);

  AABox Movable(UnitBox);
  Movable.moveTo(Vector3::UNIT * 3);
  REQUIRE(Movable.getCenter() == Vector3::UNIT * 3);

  Movable.shiftBy(Vector3(-.5f, -.5f, -.5f));
  REQUIRE(Movable.getCenter() == Vector3(2.5f, 2.5f, 2.5f));

  const Vector3 Half(.5f, .5f, .5f);
  REQUIRE(ShapeOverlap::pointBox(Half, UnitBox));
  REQUIRE_FALSE(ShapeOverlap::pointBox(Vector3::UNIT * 2, UnitBox));

  const AABox Box2(Vector3::UNIT, Vector3::UNIT * 2);
  const AABox FarBox(Vector3::UNIT * 3.1f, Vector3::UNIT * 6);
  REQUIRE(ShapeOverlap::boxBox(Box2, Movable));
  REQUIRE_FALSE(ShapeOverlap::boxBox(Movable, FarBox));

  const Vector<Vector3> ArrayPoints = {{-1.0f, -1.0f, 1.0f}, {7.0f, 8.0f, -2.0f},
                                       {1.0f, 1.1f, 1.6f},   {7.0f, 12.0f, 22.0f},
                                       {4.0f, 2.0f, 1.0f},   {6.0f, 3.1f, 22.6f}};

  const AABox FromPoints(ArrayPoints);
  REQUIRE(FromPoints.minPoint == Vector3(-1.0f, -1.0f, -2.0f));
  REQUIRE(FromPoints.maxPoint == Vector3(7.0f, 12.0f, 22.6f));
}

TEST_CASE("chUtilities - Plane") {
  REQUIRE(sizeof(Plane) == 16);

  const Plane plane1(Vector3::UNIT, Vector3::UP);
  const float DistanceToZero = plane1.planeDot(Vector3::ZERO);
  const float DistanceToThree = plane1.planeDot(Vector3::UNIT * 3.f);

  REQUIRE(DistanceToZero < 0);
  REQUIRE(DistanceToThree > 0);

  const AABox Aabox(Vector3::ZERO, Vector3::UNIT);
  const Plane Plane2AABoxTrue(Vector3::UNIT * .5f, Vector3::RIGHT);
  const Plane Plane2AABoxFalse(Vector3::UNIT * 5.f, Vector3::RIGHT);
  REQUIRE(ShapeOverlap::boxPlane(Aabox, Plane2AABoxTrue));
  REQUIRE_FALSE(ShapeOverlap::boxPlane(Aabox, Plane2AABoxFalse));

  // Planes away from the origin: w used to be subtracted twice.
  REQUIRE(ShapeOverlap::boxPlane(Aabox, Plane(Vector3(0.0f, 0.9f, 0.0f), Vector3::RIGHT)));
  REQUIRE(ShapeOverlap::boxPlane(Aabox, Plane(Vector3(0.0f, 1.0f, 0.0f), Vector3::RIGHT)));
  REQUIRE_FALSE(
      ShapeOverlap::boxPlane(Aabox, Plane(Vector3(0.0f, 1.1f, 0.0f), Vector3::RIGHT)));
  REQUIRE_FALSE(
      ShapeOverlap::boxPlane(Aabox, Plane(Vector3(0.0f, -0.1f, 0.0f), Vector3::RIGHT)));
  REQUIRE(ShapeOverlap::boxPlane(Aabox, Plane(Vector3(0.5f, 0.0f, 0.0f), Vector3::FORWARD)));
}

TEST_CASE("chUtilities - Sphere") {
  REQUIRE(sizeof(Sphere) == 16);

  const Vector<Vector3> ArrayPoints = {
      {-100.0f, -1.0f, 1.0f}, {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
      {7.0f, 10.0f, 22.0f},   {4.0f, 2.0f, 1.0f},  {6.0f, 3.1f, 22.6f},
      {-99.0f, -1.0f, 1.0f},  {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
      {7.0f, 10.0f, 22.0f},   {4.0f, 2.0f, 1.0f},  {6.0f, 3.1f, 22.6f}};

  const Sphere FromPoints(ArrayPoints);
  REQUIRE(FromPoints.radius == Approx(55.0949364f));
  REQUIRE(FromPoints.center == Vector3(-46.5000000f, 4.5f, 10.30000002f));

  const Sphere Center2(Vector3::UNIT * 3, 1);
  REQUIRE_FALSE(ShapeOverlap::pointSphere(Vector3::ZERO, Center2));
  REQUIRE(ShapeOverlap::pointSphere(Vector3::UNIT * 2.5, Center2));

  const Sphere Center3(Vector3::UNIT * 3, 2);
  const Sphere Center1(Vector3::UNIT, .5f);
  REQUIRE(ShapeOverlap::sphereSphere(Center2, Center3));
  REQUIRE_FALSE(ShapeOverlap::sphereSphere(Center3, Center1));

  const AABox Aabox(Vector3::ZERO, Vector3::UNIT);
  REQUIRE(ShapeOverlap::boxSphere(Aabox, Center1));
  REQUIRE_FALSE(ShapeOverlap::boxSphere(Aabox, Center3));
}

TEST_CASE("chUtilities - Box2D") {
  REQUIRE(sizeof(Sphere) == 16);

  const Box2D UnityBox(Vector2::ZERO, Vector2::UNIT);
  REQUIRE(UnityBox.getCenter() == Vector2(.5f, .5f));
  REQUIRE(UnityBox.getSize() == Vector2(1.f, 1.f));
  REQUIRE(UnityBox.getExtent() == Vector2(.5f, .5f));

  const Box2D AnotherBox(Vector2::UNIT, Vector2::UNIT * 3);
  REQUIRE(AnotherBox.getCenter() == Vector2(2.f, 2.f));
  REQUIRE(AnotherBox.getSize() == Vector2(2.f, 2.f));
  REQUIRE(AnotherBox.getExtent() == Vector2(1.f, 1.f));
}

TEST_CASE("chUtilities - SphereBoxBounds") {
  REQUIRE(sizeof(SphereBoxBounds) == 28);

  const Vector<Vector3> ArrayPoints = {
      {-100.0f, -1.0f, 1.0f}, {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
      {7.0f, 10.0f, 22.0f},   {4.0f, 2.0f, 1.0f},  {6.0f, 3.1f, 22.6f},
      {-99.0f, -1.0f, 1.0f},  {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
      {7.0f, 10.0f, 22.0f},   {4.0f, 2.0f, 1.0f},  {6.0f, 3.1f, 22.6f}};

  const SphereBoxBounds FromPoints(ArrayPoints);
  const SphereBoxBounds FromSphereBox(Vector3::UNIT, Vector3::UNIT * 5, 6.f);
  const SphereBoxBounds FromSphereBoxTRUE(Vector3::UNIT * -46.f, Vector3::UNIT * 5, 6.f);
  REQUIRE(FromPoints.center == Vector3(-46.5000000f, 4.5f, 10.30000002f));
  REQUIRE(FromPoints.boxExtent == Vector3(53.5f, 5.5f, 12.30000002f));

  // The centers are about 48.5 apart and the radii add up to about 60.6. The old code
  // compared the squared distance with sqrt of the radii, so it said they did not touch.
  REQUIRE(ShapeOverlap::sphereSphere(FromPoints, FromSphereBox));
  REQUIRE_FALSE(ShapeOverlap::boxBox(FromPoints, FromSphereBoxTRUE));

  // Radii 1 and 2 with centers 3 apart touch; one unit further they do not.
  const SphereBoxBounds Touching1(Vector3::ZERO, Vector3::UNIT, 1.0f);
  const SphereBoxBounds Touching2(Vector3(3.0f, 0.0f, 0.0f), Vector3::UNIT, 2.0f);
  const SphereBoxBounds Apart(Vector3(4.0f, 0.0f, 0.0f), Vector3::UNIT, 2.0f);
  REQUIRE(ShapeOverlap::sphereSphere(Touching1, Touching2));
  REQUIRE_FALSE(ShapeOverlap::sphereSphere(Touching1, Apart));
}

TEST_CASE("chUtilities - Utilities") {
  class Submodule : public Module<Submodule>
  {
   public:
    int32 TestNumber = 11552;
  };
  REQUIRE_THROWS_AS(Submodule::instance(), InternalErrorException);
  REQUIRE_THROWS_AS(Submodule::instancePtr(), InternalErrorException);

  Submodule::startUp<Submodule>();
  REQUIRE_NOTHROW(Submodule::instance());
  REQUIRE_NOTHROW(Submodule::instancePtr());

  REQUIRE(Submodule::instance().TestNumber == 11552);

  REQUIRE_THROWS_AS(Submodule::startUp<Submodule>(), InternalErrorException);

  Submodule::shutDown();
  REQUIRE_THROWS_AS(Submodule::instance(), InternalErrorException);
  REQUIRE_THROWS_AS(Submodule::instancePtr(), InternalErrorException);

  // DynamicLibraryManager::startUp();
  // WeakPtr<DynamicLibrary> TestDll =
  // DynamicLibraryManager::instance().loadDynLibrary("DllTest"); SPtr<DynamicLibrary> RealPtr
  // = TestDll.lock(); REQUIRE(RealPtr);

  // using SymbolFromDll = void(*)();
  // SymbolFromDll func = reinterpret_cast<SymbolFromDll>(RealPtr->getSymbol("testFunction"));

  // REQUIRE(func);
  // func();

  Event<int32(int32, float)> Onsomething;
  HEvent listener1 = Onsomething.connect([](int32 a, float b) -> int32 {
    REQUIRE(a == 10);
    REQUIRE(b == 125.55f);
    return 1;
  });

  struct Test {
    Test(int32 a, float b) : A(a), B(b) {}
    int32 A;
    float B;

    int32
    foo(int32 a, float b) {
      REQUIRE(a == 10);
      REQUIRE(b == 125.55f);
      return 0;
    }
  };

  auto* TestClass = new Test(123, 35445.64565f);

  HEvent listener2 = Onsomething.connect([&TestClass](int32 a, float b) -> int32 {
    REQUIRE(TestClass->A == 123);
    REQUIRE(TestClass->B == 35445.64565f);
    TestClass->foo(a, b);
    return 1;
  });

  HEvent listener3 = Onsomething.connect(
      std::bind(&Test::foo, TestClass, std::placeholders::_1, std::placeholders::_2));

  Onsomething(10, 125.55f);
}

TEST_CASE("chUtilities - EventSystem") {
  SECTION("More listeners than the inline capacity") {
    Event<void(int32)> onValue;
    int32 total = 0;
    Vector<HEvent> handles;
    for (int32 i = 0; i < 40; ++i) {
      handles.push_back(onValue.connect([&total](int32 value) { total += value; }));
    }
    onValue(2);
    REQUIRE(total == 80);
  }

  SECTION("Custom inline capacity") {
    Event<void(int32), 4> onValue;
    int32 total = 0;
    Vector<HEvent> handles;
    for (int32 i = 0; i < 4; ++i) {
      handles.push_back(onValue.connect([&total](int32 value) { total += value; }));
    }
    onValue(1);
    REQUIRE(total == 4);

    for (int32 i = 0; i < 6; ++i) {
      handles.push_back(onValue.connect([&total](int32 value) { total += value; }));
    }
    onValue(1);
    REQUIRE(total == 14);
  }

  SECTION("Arguments by value reach every listener") {
    Event<void(String)> onText;
    int32 matches = 0;
    HEvent first = onText.connect([&matches](String text) { matches += text == "hello"; });
    HEvent second = onText.connect([&matches](String text) { matches += text == "hello"; });
    onText(String("hello"));
    REQUIRE(matches == 2);
  }

  SECTION("Disconnect during a fire") {
    Event<void()> onFire;
    int32 firstCalls = 0;
    int32 secondCalls = 0;
    HEvent second;
    HEvent first = onFire.connect([&]() {
      ++firstCalls;
      first.disconnect();
      second.disconnect();
    });
    second = onFire.connect([&]() { ++secondCalls; });

    // The second listener was already pinned, so it still runs during this fire.
    onFire();
    REQUIRE(firstCalls == 1);
    REQUIRE(secondCalls == 1);

    onFire();
    REQUIRE(firstCalls == 1);
    REQUIRE(secondCalls == 1);
  }

  SECTION("Connect during a fire") {
    Event<void()> onFire;
    int32 lateCalls = 0;
    HEvent late;
    HEvent first = onFire.connect([&]() {
      if (!late.isValid()) {
        late = onFire.connect([&]() { ++lateCalls; });
      }
    });

    onFire();
    REQUIRE(lateCalls == 0);
    onFire();
    REQUIRE(lateCalls == 1);
  }

  SECTION("Clear during a fire") {
    Event<void()> onFire;
    int32 calls = 0;
    HEvent first = onFire.connect([&]() {
      ++calls;
      onFire.clear();
    });
    HEvent second = onFire.connect([&]() { ++calls; });

    onFire();
    REQUIRE(calls == 2);
    onFire();
    REQUIRE(calls == 2);
  }

  SECTION("Fire from inside a callback") {
    Event<void(int32)> onDepth;
    int32 calls = 0;
    HEvent handle = onDepth.connect([&](int32 depth) {
      ++calls;
      if (depth < 3) {
        onDepth(depth + 1);
      }
    });
    onDepth(0);
    REQUIRE(calls == 4);
  }

  SECTION("Moving keeps the listeners") {
    int32 total = 0;
    UnorderedMap<int32, Event<void(int32)>> events;
    HEvent handle;
    {
      Event<void(int32)> onValue;
      handle = onValue.connect([&total](int32 value) { total += value; });
      events.emplace(0, std::move(onValue));
    }
    events[0](3);
    REQUIRE(total == 3);

    Event<void(int32)> other;
    other = std::move(events[0]);
    other(2);
    REQUIRE(total == 5);
    REQUIRE(handle.isValid());
  }
}

TEST_CASE("chUtilities - Logger") {
  CH_LOG_DECLARE_STATIC(LoggerTestLog, All);
  REQUIRE(Logger::findCategory("LoggerTestLog") == &LoggerTestLog);

  // Logged before the Logger starts, and added to its buffer when it starts.
  CH_LOG_INFO(LoggerTestLog, "Early");

  Logger::startUp();
  Logger& logger = Logger::instance();
  REQUIRE(logger.getBufferedLogs().back()->message == "Early");

  logger.setConsoleOutput(false);
  logger.setBufferingEnabled(true, 3);

  for (int32 i = 0; i < 5; ++i) {
    CH_LOG_INFO(LoggerTestLog, "Message {0}", i);
  }

  // The ring buffer keeps the last 3 entries, oldest first.
  Vector<SPtr<const LogBufferEntry>> buffered = logger.getBufferedLogs();
  REQUIRE(buffered.size() == 3);
  REQUIRE(buffered[0]->message == "Message 2");
  REQUIRE(buffered[1]->message == "Message 3");
  REQUIRE(buffered[2]->message == "Message 4");

  const LogBufferEntry& entry = *buffered[2];
  REQUIRE(entry.verbosity == LogVerbosity::Info);
  REQUIRE(entry.category == "LoggerTestLog");
  REQUIRE(entry.sourceFile == "chUtilitiesMain.cpp");
  REQUIRE(entry.sourceLine > 0);
  REQUIRE(StringView(entry.timestamp).size() == 23);
  REQUIRE(entry.timestamp[4] == '-');
  REQUIRE(entry.timestamp[10] == ' ');
  REQUIRE(entry.timestamp[19] == '.');

  // Replays the buffer in order, then receives the same shared entry as the buffer.
  Vector<SPtr<const LogBufferEntry>> received;
  HEvent listener = logger.onLogWritten(
      [&received](const SPtr<const LogBufferEntry>& logEntry) {
        received.push_back(logEntry);
      },
      true);
  REQUIRE(received.size() == 3);
  REQUIRE(received[0]->message == "Message 2");

  CH_LOG_WARNING(LoggerTestLog, "Shared");
  REQUIRE(received.size() == 4);
  REQUIRE(received.back() == logger.getBufferedLogs().back());
  Logger::disconnectLogListener(listener);

  // Shrinking the buffer keeps the newest entries.
  logger.setBufferingEnabled(true, 2);
  buffered = logger.getBufferedLogs();
  REQUIRE(buffered.size() == 2);
  REQUIRE(buffered[0]->message == "Message 4");
  REQUIRE(buffered[1]->message == "Shared");

  int32 lateCalls = 0;
  HEvent lateListener = logger.onLogWritten(
      [&lateCalls](const SPtr<const LogBufferEntry>&) { ++lateCalls; });

  Logger::shutDown();

  // After shutDown logging only reaches the console, and a listener can still be released.
  CH_LOG_INFO(LoggerTestLog, "After shut down");
  REQUIRE(lateCalls == 0);
  Logger::disconnectLogListener(lateListener);
  REQUIRE_FALSE(lateListener.isValid());
}

TEST_CASE("chUtilities - ContainsIgnoreCase") {
  REQUIRE(StringUtils::containsIgnoreCase("Texture_Wood", "wood"));
  REQUIRE(StringUtils::containsIgnoreCase("Texture_Wood", "TEXTURE"));
  REQUIRE(StringUtils::containsIgnoreCase("Texture_Wood", "e_w"));
  REQUIRE(StringUtils::containsIgnoreCase("abc", "abc"));
  REQUIRE(StringUtils::containsIgnoreCase("abc", ""));
  REQUIRE(StringUtils::containsIgnoreCase("", ""));
  REQUIRE_FALSE(StringUtils::containsIgnoreCase("abc", "abcd"));
  REQUIRE_FALSE(StringUtils::containsIgnoreCase("", "a"));
  REQUIRE_FALSE(StringUtils::containsIgnoreCase("Texture_Wood", "stone"));
  REQUIRE_FALSE(StringUtils::containsIgnoreCase("aab", "abb"));

  REQUIRE(StringUtils::equalsIgnoreCase("PNG", "png"));
  REQUIRE(StringUtils::equalsIgnoreCase("", ""));
  REQUIRE_FALSE(StringUtils::equalsIgnoreCase("png", "pn"));
  REQUIRE_FALSE(StringUtils::equalsIgnoreCase("png", "jpg"));
}

TEST_CASE("chUtilities - Format") {
  enum class TestEnum : int32 { A = 3, B = -2 };

  SECTION("default output") {
    REQUIRE(StringUtils::format("{} {} {}", 42, -7, 18446744073709551615ull) ==
            "42 -7 18446744073709551615");
    REQUIRE(StringUtils::format("{} {} {} {}", 1.0, 0.1, 1e20, -2.5f) == "1 0.1 1e+20 -2.5");
    REQUIRE(StringUtils::format("{}|{}|{}", 'a', true, false) == "a|true|false");
    REQUIRE(StringUtils::format("{}", static_cast<uint8>(65)) == "65");
    REQUIRE(StringUtils::format("{} {}", TestEnum::A, TestEnum::B) == "3 -2");
    REQUIRE(StringUtils::format("{1}-{0}-{1}", "x", String("y")) == "y-x-y");
    REQUIRE(StringUtils::format("{} {}", Path("a\\b"), static_cast<const ANSICHAR*>(nullptr)) ==
            "a/b (null)");
    REQUIRE(StringUtils::format("{{{}}}", 5) == "{5}");
    REQUIRE(StringUtils::toString(0.5) == "0.5");
  }

  SECTION("specs") {
    REQUIRE(StringUtils::format("{:.2f}", 3.14159) == "3.14");
    REQUIRE(StringUtils::format("{:f}", 1.0) == "1.000000");
    REQUIRE(StringUtils::format("{:.3e}", 12345.678) == "1.235e+04");
    REQUIRE(StringUtils::format("{:.3}", 3.14159) == "3.14");
    REQUIRE(StringUtils::format("{:.1f}", 1e300).size() == 303);
    REQUIRE(StringUtils::format("{:x} {:X} {:b} {:o}", 255, 255u, 5, 8) == "ff FF 101 10");
    REQUIRE(StringUtils::format("{:08x}", 0xbeefu) == "0000beef");
    REQUIRE(StringUtils::format("{:05}", -42) == "-0042");
    REQUIRE(StringUtils::format("{:6.2f}", -1.5) == " -1.50");
    REQUIRE(StringUtils::format("[{:>5}][{:<5}][{:^5}]", 1, 2, 3) == "[    1][2    ][  3  ]");
    REQUIRE(StringUtils::format("[{:6}][{:>6}][{:*^7}]", "ab", "ab", "ab") ==
            "[ab    ][    ab][**ab***]");
    REQUIRE(StringUtils::format("[{0:>3}][{0}]", 'c') == "[  c][c]");
    REQUIRE(StringUtils::format("{:2}", "longer") == "longer");
  }
}

TEST_CASE("chUtilities - ToChars") {
  ANSICHAR integer[StringUtils::MAX_INTEGER_CHARS];
  REQUIRE(StringUtils::toChars(integer, 0) == "0");
  REQUIRE(StringUtils::toChars(integer, static_cast<uint32>(437)) == "437");
  REQUIRE(StringUtils::toChars(integer, static_cast<uint8>(255)) == "255");
  REQUIRE(StringUtils::toChars(integer, static_cast<int64>(-9223372036854775807ll - 1)) ==
          "-9223372036854775808");
  REQUIRE(StringUtils::toChars(integer, static_cast<uint64>(18446744073709551615ull)) ==
          "18446744073709551615");

  ANSICHAR number[StringUtils::MAX_FLOAT_CHARS];
  REQUIRE(StringUtils::toChars(number, 0.1) == "0.1");
  REQUIRE(StringUtils::toChars(number, -2.5f) == "-2.5");
  REQUIRE(StringUtils::toChars(number, -2.2250738585072014e-308) ==
          "-2.2250738585072014e-308");
}

TEST_CASE("chUtilities - Algorithm") {
  Vector<int32> values = {5, 1, 4, 1, 3};
  REQUIRE(Algorithm::contains(values, 4));
  REQUIRE_FALSE(Algorithm::contains(values, 7));

  REQUIRE(Algorithm::removeAll(values, 1) == 2);
  REQUIRE(values == Vector<int32>{5, 4, 3});
  REQUIRE(Algorithm::removeFirst(values, 4));
  REQUIRE_FALSE(Algorithm::removeFirst(values, 4));
  REQUIRE(values == Vector<int32>{5, 3});

  values = {3, 1, 2};
  Algorithm::sort(values);
  REQUIRE(values == Vector<int32>{1, 2, 3});
  Algorithm::sort(values, [](int32 a, int32 b) { return a > b; });
  REQUIRE(values == Vector<int32>{3, 2, 1});

  const Vector<uint64> sorted = {2, 4, 4, 8};
  REQUIRE(Algorithm::lowerBound(sorted, uint64{0}) == 0);
  REQUIRE(Algorithm::lowerBound(sorted, uint64{4}) == 1);
  REQUIRE(Algorithm::lowerBound(sorted, uint64{5}) == 3);
  REQUIRE(Algorithm::lowerBound(sorted, uint64{9}) == 4);

  Vector<int32> ring = {4, 5, 1, 2, 3};
  Algorithm::rotateToFront(ring, 2);
  REQUIRE(ring == Vector<int32>{1, 2, 3, 4, 5});
  Algorithm::rotateToFront(ring, ring.size());
  REQUIRE(ring == Vector<int32>{1, 2, 3, 4, 5});
}

TEST_CASE("chUtilities - Unicode") {
  // "a", e acute, euro sign, and an emoji that needs a surrogate pair in UTF-16.
  const String utf8 = "a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
  const U16String utf16 = u"a\xE9\x20AC\U0001F600";
  const U32String utf32 = U"a\xE9\x20AC\U0001F600";
  const String replacement = "\xEF\xBF\xBD";

  SECTION("valid text") {
    REQUIRE(UTF8::toUTF16(utf8) == utf16);
    REQUIRE(UTF8::fromUTF16(utf16) == utf8);
    REQUIRE(UTF8::toUTF32(utf8) == utf32);
    REQUIRE(UTF8::fromUTF32(utf32) == utf8);
    REQUIRE(UTF8::fromWide(UTF8::toWide(utf8)) == utf8);
    REQUIRE(UTF8::toUTF16("").empty());
  }

  SECTION("invalid UTF-8") {
    // Overlong '/', 5 byte form, cut sequence before 'A', lone continuation byte, encoded
    // surrogate.
    REQUIRE(UTF8::toUTF32("\xC0\xAF") == U"\xFFFD");
    REQUIRE(UTF8::toUTF32("\xF8\x88\x80\x80\x80").size() == 5);
    REQUIRE(UTF8::toUTF32("\xE2\x82" "A") == U"\xFFFD" "A");
    REQUIRE(UTF8::toUTF32("\x80" "b") == U"\xFFFD" "b");
    REQUIRE(UTF8::toUTF32("\xED\xA0\x80") == U"\xFFFD");
  }

  SECTION("invalid UTF-16 and UTF-32") {
    const U16String loneHigh = {0xD800, u'A'};
    const U16String loneLow = {0xDC00};
    REQUIRE(UTF8::fromUTF16(loneHigh) == replacement + "A");
    REQUIRE(UTF8::fromUTF16(loneLow) == replacement);

    const U32String outOfRange = {0xD800, 0x110000};
    REQUIRE(UTF8::fromUTF32(outOfRange) == replacement + replacement);
    REQUIRE(UTF8::toUTF16(replacement) == u"\xFFFD");
  }
}

TEST_CASE("chUtilities - DataStream getAsString") {
  auto readText = [](Vector<uint8> bytes) {
    MemoryDataStream stream(bytes.data(), bytes.size(), false);
    return stream.getAsString();
  };
  const String expected = "A\xC3\xA9";

  REQUIRE(readText({'A', 0xC3, 0xA9}) == expected);
  REQUIRE(readText({0xEF, 0xBB, 0xBF, 'A', 0xC3, 0xA9}) == expected);
  REQUIRE(readText({0xFF, 0xFE, 'A', 0x00, 0xE9, 0x00}) == expected);
  REQUIRE(readText({0xFE, 0xFF, 0x00, 'A', 0x00, 0xE9}) == expected);
  REQUIRE(readText({0xFF, 0xFE, 0x00, 0x00, 'A', 0, 0, 0, 0xE9, 0, 0, 0}) == expected);
  REQUIRE(readText({0x00, 0x00, 0xFE, 0xFF, 0, 0, 0, 'A', 0, 0, 0, 0xE9}) == expected);
  REQUIRE(readText({0xFF, 0xFE, 'A', 0x00, 0xE9}) == "A");
  REQUIRE(readText({}).empty());
}

TEST_CASE("chUtilities - MemoryDataStream") {
  SECTION("read and write stop at the end") {
    MemoryDataStream stream(4);
    const uint8 source[6] = {1, 2, 3, 4, 5, 6};
    REQUIRE(stream.write(source, sizeof(source)) == 4);
    REQUIRE(stream.write(source, 1) == 0);

    stream.seek(2);
    uint8 target[6] = {};
    REQUIRE(stream.read(target, sizeof(target)) == 2);
    REQUIRE(target[0] == 3);
    REQUIRE(target[1] == 4);
    REQUIRE(target[2] == 0);
    REQUIRE(stream.read(target, sizeof(target)) == 0);
    REQUIRE(stream.isAtEnd());
  }

  SECTION("copies keep their data") {
    auto source = chMakeShared<MemoryDataStream>(3);
    const uint8 bytes[3] = {7, 8, 9};
    source->write(bytes, sizeof(bytes));

    SPtr<DataStream> clone = source->clone();
    source->seek(0);
    MemoryDataStream copy{SPtr<DataStream>(source)};
    source->close();

    uint8 fromClone[3] = {};
    REQUIRE(clone->read(fromClone, sizeof(fromClone)) == 3);
    REQUIRE(fromClone[2] == 9);

    uint8 fromCopy[3] = {};
    REQUIRE(copy.size() == 3);
    REQUIRE(copy.read(fromCopy, sizeof(fromCopy)) == 3);
    REQUIRE(fromCopy[0] == 7);
  }

  SECTION("a file stream copies any stream") {
    const Path memoryFile("chStreamTest_memory.bin");
    const Path fileFile("chStreamTest_file.bin");

    auto memory = chMakeShared<MemoryDataStream>(3);
    const uint8 bytes[3] = {'a', 'b', 'c'};
    memory->write(bytes, sizeof(bytes));
    FileSystem::dumpMemStreamIntoFile(memory, memoryFile);

    SPtr<DataStream> fromMemory = FileSystem::openFile(memoryFile);
    REQUIRE(fromMemory);
    REQUIRE(fromMemory->getAsString() == "abc");

    FileSystem::dumpMemStreamIntoFile(fromMemory, fileFile);
    fromMemory->close();
    SPtr<DataStream> fromFile = FileSystem::openFile(fileFile);
    REQUIRE(fromFile);
    REQUIRE(fromFile->getAsString() == "abc");
    fromFile->close();

    REQUIRE(FileSystem::remove(memoryFile));
    REQUIRE(FileSystem::remove(fileFile));
  }

  SECTION("a missing file does not throw") {
    REQUIRE_NOTHROW(FileDataStream(Path("chStreamTest_missing.bin")));
    REQUIRE_FALSE(FileDataStream(Path("chStreamTest_missing.bin")).isOpen());
    REQUIRE(FileSystem::openFile(Path("chStreamTest_missing.bin")) == nullptr);
  }
}

TEST_CASE("chUtilities - Path matches std::filesystem") {
  namespace fs = std::filesystem;
  const Vector<String> cases = {
    "", "/", "//", "///a", "//a", "C:", "C:/", "C://a", "C:/a", "C:a", "c:/a/b.txt", "x:",
    "1:/a", "//server", "//server/", "//server/share/x.y", "//?/C:/x", "//./pipe",
    "a", "a/", "a/b", "a//b", "a/b/", "a/b//", "/a", "/a/", "/a/b.c", ".", "..", "a/..",
    "a/.", "./a", ".bashrc", "dir/.bashrc", ".a.b", "a.b.c", "a.", "/a.b/c", "../x.tar.gz",
    "Assets/Textures/wood.png",
  };

  for (const String& text : cases) {
    INFO("path: \"" << text << "\"");
    const Path path(text);
    const fs::path expected(text);
    CHECK(path.getFileName() == expected.filename().generic_string());
    CHECK(path.getFileName(false) == expected.stem().generic_string());
    CHECK(path.getExtension() == expected.extension().generic_string());
    CHECK(path.getDirectory().toString() == expected.parent_path().generic_string());
    CHECK(path.isRelative() == expected.is_relative());

    for (const String& other : cases) {
      INFO("joined with: \"" << other << "\"");
      const String joined = (expected / other).generic_string();
      CHECK(path.join(Path(other)).toString() == joined);
      CHECK((path / other).toString() == joined);

      Path inPlace = path;
      inPlace /= Path(other);
      CHECK(inPlace.toString() == joined);
    }
  }

  SECTION("backslashes become separators") {
    REQUIRE(Path("a\\b\\c.txt").toString() == "a/b/c.txt");
    REQUIRE((Path("a") / String("b\\c")).toString() == "a/b/c");
  }

  SECTION("several paths join in order") {
    REQUIRE(Path(Path("a"), Path("b/"), Path("c")).toString() == "a/b/c");
    REQUIRE(Path(Vector<Path>{Path("a"), Path("/b"), Path("c")}).toString() == "/b/c");
    Path self("a");
    self /= self;
    REQUIRE(self.toString() == "a/a");
  }
}

// TEST_CASE("chUtilities - StringAndUTF8") {
//     const U16String TestWString(UTF8::toUTF16("Created as wide string"));
//     const String WellPerformedConvertion("Created as wide string");

//     REQUIRE(UTF8::fromWide(TestWString) == WellPerformedConvertion);
//     REQUIRE(UTF8::toWide(WellPerformedConvertion) == TestWString);

//     const String ReplaceTestString("Test-string-that-should-replace-all-hyphens");
//     const String WellPerformedReplacing("Test string that should replace all hyphens");
//     REQUIRE(StringUtils::replaceAllChars(ReplaceTestString, '-', ' ') ==
//     WellPerformedReplacing);

//     const String ReplaceSubStringtest("Test that asdfg contains asdfg some asdfg substrings
//     to erase"); const String WellPerformedSubStrReplace("Test that sd contains sd some sd
//     substrings to erase"); REQUIRE(StringUtils::replaceAllSubStr(ReplaceSubStringtest, "asdfg",
//     "sd") == WellPerformedSubStrReplace);

//     const String ToSplitChar("This-is-a-test-that-should-split-by-hyphens");
//     REQUIRE(StringUtils::splitString(ToSplitChar, '-').size() == 9);

//     const String
//     ToSplitString("This123is123a123test123that123should123split123by123hyphen");
//     REQUIRE(StringUtils::splitString(ToSplitString, "123").size() == 9);

//     String Formated = StringUtils::format("hello world, {0}, {0}, {1}, {2}, {3}",
//         123, 1.23, "123", String("123"));

//     const String Anwser("hello world, 123, 123, 1.230000, 123, 123");
//     REQUIRE(Anwser == Formated);

//     // Platform-specific paths
// #if USING(CH_PLATFORM_WIN32)
//     const Path TestPath("C:\\Users\\Public\\");
//     Path WrongTest("C:/Users/Public/NoExist/");
//     const Path ToCreate("C:/Users/Public/Test/");
//     const Path ToCreateMany("C:/Users/Public/Test/a/b/c/");
//     REQUIRE(TestPath.toString() == "C:/Users/Public/");
//     REQUIRE(WrongTest.toString() == "C:/Users/Public/NoExist/");
// #elif USING(CH_PLATFORM_LINUX)
//     const Path TestPath("/home/accelmr/Public/");
//     Path WrongTest("/home/accelmr/Public/NoExist/");
//     const Path ToCreate("/home/accelmr/Public/Test/");
//     const Path ToCreateMany("/home/accelmr/Public/Test/a/b/c/");

//     REQUIRE(TestPath.toString() == "/home/accelmr/Public/");
//     REQUIRE(WrongTest.toString() == "/home/accelmr/Public/NoExist/");
// #endif

//     Path Relativepath2("./Here/path");
//     Path Relativepath3("Here/path");

//     REQUIRE(Relativepath2.toString() == "./Here/path");
//     REQUIRE(Relativepath3.toString() == "Here/path");

//     REQUIRE(FileSystem::createDirectory(ToCreate));

//     REQUIRE(FileSystem::createDirectories(ToCreateMany));

//     SPtr<DataStream> memStream = chMakeShared<MemoryDataStream>(8);
//     REQUIRE(memStream->size() == 8);

//     int Test1110 = 110;
//     memStream->write(&Test1110, sizeof(int));
//     *memStream << 18;

//     SIZE_T moved = memStream->tell();
//     REQUIRE(moved == 8);
//     memStream->seek(0);

//     int InTest110 = 0;
//     memStream->read(&InTest110, sizeof(int));
//     REQUIRE(InTest110 == 110);

//     int Intest18;
//     *memStream >> Intest18;
//     REQUIRE(Intest18 == 18);

//     REQUIRE_FALSE(memStream->isFile());
//     REQUIRE(memStream->isReadable());
//     REQUIRE(memStream->isWriteable());

//     Path testFilePath(ToCreateMany.toString() + "testFile.test");
//     FileSystem::dumpMemStreamIntoFile(memStream, testFilePath);

//     auto readFileSteam = FileSystem::openFile(testFilePath);
//     REQUIRE(readFileSteam);

//     int FileInTest110 = 0;
//     readFileSteam->read(&FileInTest110, sizeof(int));
//     REQUIRE(FileInTest110 == 110);

//     int FileIntest18;
//     *readFileSteam >> FileIntest18;
//     REQUIRE(FileIntest18 == 18);

//     memStream.reset();
//     REQUIRE_FALSE(memStream);

//     readFileSteam->close();

//     const Path newFilePath(ToCreateMany.toString() + "testFile2.test2");
//     SPtr<DataStream> newFile = FileSystem::createAndOpenFile(newFilePath);

//     *newFile << "This is a test to see if write() is ok" << " concat ok " << "\n";
//     *newFile << "This must be a new line";
//     newFile->close();

//     auto readFile = FileSystem::openFile(newFilePath);
//     String fromFile = readFile->getAsString();

//     const String mustBe("This is a test to see if write() is ok\xEF\xBB\xBF concat ok
//     \xEF\xBB\xBF\n\xEF\xBB\xBFThis must be a new line"); REQUIRE(mustBe == fromFile);

//     readFile->close();

//     REQUIRE_THROWS_AS(FileSystem::remove(ToCreate), std::exception);
//     REQUIRE(FileSystem::removeAll(ToCreate));

//     /*
//     const Path DebugFile("C:/Users/Public/debug.txt");
//     CH_LOG_DEBUG("TEST");
//     CH_LOG_ERROR("TEST");
//     CH_LOG_WARNING("TEST");
//     g_Debug().saveLog(DebugFile);*/
// }

TEST_CASE("chUtilities - RandomNumbers") {
  uint32 randomNumber = 0;
  Random rnd = Random();

  randomNumber = rnd.getPseudoRandom() % 1000;
  REQUIRE(randomNumber == 363);

  for (uint8 i = 0; i < 10; i++) {
    randomNumber = rnd.getPseudoRandom() % 1000;
  }

  REQUIRE(randomNumber == 735);
}

TEST_CASE("chUtilities - FileSystem") {
  SECTION("absolutePath removes dot segments") {
    REQUIRE(FileSystem::absolutePath(Path("a/../b")) == FileSystem::absolutePath(Path("b")));
    REQUIRE(FileSystem::absolutePath(Path("./a/")) == FileSystem::absolutePath(Path("a")));
    REQUIRE_FALSE(FileSystem::absolutePath(Path("a")).isRelative());
  }

  SECTION("isSubPath compares whole folder names") {
    REQUIRE(FileSystem::isSubPath(Path("Assets"), Path("Assets/Textures/a.chAss")));
    REQUIRE(FileSystem::isSubPath(Path("Assets"), Path("Assets")));
    REQUIRE(FileSystem::isSubPath(Path("Assets"), FileSystem::absolutePath(Path("Assets/x"))));
    REQUIRE(FileSystem::isSubPath(Path("Other/../Assets"), Path("Assets/x")));
    REQUIRE_FALSE(FileSystem::isSubPath(Path("Assets"), Path("Assets2/x")));
    REQUIRE_FALSE(FileSystem::isSubPath(Path("Assets"), Path("Assets/../x")));
    REQUIRE_FALSE(FileSystem::isSubPath(Path("Assets/x"), Path("Assets")));
  }

  SECTION("relative paths resolve against the base directory") {
    const Path workingDirectory = FileSystem::getBaseDirectory();
    const Path executableDirectory = FileSystem::getExecutableDirectory();
    REQUIRE_FALSE(executableDirectory.isRelative());
    REQUIRE(FileSystem::isDirectory(executableDirectory));

    FileSystem::setBaseDirectory(executableDirectory);
    REQUIRE(FileSystem::getBaseDirectory() == executableDirectory);
    REQUIRE(FileSystem::absolutePath(Path("Assets/a")) ==
            executableDirectory.join(Path("Assets/a")));
    REQUIRE(FileSystem::toRelativePath(executableDirectory.join(Path("Assets/a"))) ==
            Path("Assets/a"));
    REQUIRE(FileSystem::toRelativePath(executableDirectory.getDirectory()) == Path(".."));

    FileSystem::setBaseDirectory(workingDirectory);
    REQUIRE(FileSystem::getBaseDirectory() == workingDirectory);
  }

  SECTION("mounted directories are layered by priority") {
    const Path root("chMountTestDir");
    FileSystem::removeAll(root);

    auto writeFile = [](const Path& path, uint8 value) {
      SPtr<DataStream> file = FileSystem::createAndOpenFile(path);
      REQUIRE(file != nullptr);
      file->write(&value, 1);
      file->close();
    };
    writeFile(Path("chMountTestDir/Base/shared.bin"), 1);
    writeFile(Path("chMountTestDir/Base/baseOnly.bin"), 2);
    writeFile(Path("chMountTestDir/Mod/shared.bin"), 3);
    writeFile(Path("chMountTestDir/Engine/engine.bin"), 4);

    REQUIRE(FileSystem::mount("TestGame", Path("chMountTestDir/Base"), 0, true));
    REQUIRE(FileSystem::mount("TestGame", Path("chMountTestDir/Mod"), 10));
    REQUIRE(FileSystem::mount("TestEngine", Path("chMountTestDir/Engine")));

    REQUIRE(FileSystem::isVirtual(Path("/TestGame/shared.bin")));
    REQUIRE_FALSE(FileSystem::isVirtual(Path("/NotMounted/shared.bin")));
    REQUIRE_FALSE(FileSystem::isVirtual(Path("/TestGame/../shared.bin")));

    REQUIRE(FileSystem::fastRead(Path("/TestGame/shared.bin"))[0] == 3);
    REQUIRE(FileSystem::fastRead(Path("/TestGame/baseOnly.bin"))[0] == 2);
    REQUIRE(FileSystem::isFile(Path("/TestEngine/engine.bin")));

    writeFile(Path("/TestGame/new.bin"), 5);
    REQUIRE(FileSystem::isFile(Path("chMountTestDir/Base/new.bin")));
    REQUIRE(FileSystem::createAndOpenFile(Path("/TestEngine/x.bin")) == nullptr);

    Vector<Path> files, directories;
    FileSystem::getChildren(Path("/TestGame"), files, directories);
    REQUIRE(files.size() == 3);
    for (const Path& file : files) {
      REQUIRE(FileSystem::isSubPath(Path("/TestGame"), file));
    }

    REQUIRE(FileSystem::toVirtualPath(Path("chMountTestDir/Base/baseOnly.bin")) ==
            Path("/TestGame/baseOnly.bin"));
    REQUIRE(FileSystem::toVirtualPath(Path("chMountTestDir/Other.bin")).empty());
    REQUIRE(FileSystem::isSubPath(Path("/TestGame"), Path("chMountTestDir/Base/x")));
    REQUIRE_FALSE(FileSystem::isSubPath(Path("/TestGame"), Path("/TestEngine/engine.bin")));
    REQUIRE(FileSystem::isSubPath(Path("/TestGame"), Path("/TestGame")));
    REQUIRE_FALSE(FileSystem::isSubPath(Path("/TestGame"), Path("/TestGameX/a")));

    // Text that is not normalized takes the slow path and must give the same answers.
    REQUIRE(FileSystem::isVirtual(Path("/Other/../TestGame/shared.bin")));
    REQUIRE(FileSystem::toVirtualPath(Path("/TestGame/./sub//x.bin/")) ==
            Path("/TestGame/sub/x.bin"));
    REQUIRE(FileSystem::toVirtualPath(Path("/TestGame/sub/x.bin")) ==
            Path("/TestGame/sub/x.bin"));
    REQUIRE(FileSystem::toVirtualPath(Path("chMountTestDir/Mod/../Base/baseOnly.bin")) ==
            Path("/TestGame/baseOnly.bin"));
    REQUIRE(FileSystem::fastRead(Path("/TestGame/sub/../shared.bin"))[0] == 3);

    writeFile(Path("chMountTestDir/Mod/sub/deep.bin"), 6);
    writeFile(Path("chMountTestDir/Base/sub/deep.bin"), 7);
    Vector<Path> recursiveFiles;
    FileSystem::forEachFileChildRecursive(Path("/TestGame"), [&](const Path& file) {
      recursiveFiles.push_back(file);
    });
    // shared.bin and sub/deep.bin are in both layers but listed once.
    REQUIRE(recursiveFiles.size() == 5);
    REQUIRE(std::find(recursiveFiles.begin(), recursiveFiles.end(),
                      Path("/TestGame/sub/deep.bin")) != recursiveFiles.end());
    REQUIRE(FileSystem::fastRead(Path("/TestGame/sub/deep.bin"))[0] == 6);

    // With one layer the path is not checked on disk, so a missing file still resolves
    // to where it would be.
    REQUIRE(FileSystem::absolutePath(Path("/TestEngine/missing.bin")) ==
            FileSystem::absolutePath(Path("chMountTestDir/Engine/missing.bin")));
    Vector<Path> engineFiles;
    FileSystem::forEachFileChildRecursive(Path("/TestEngine"), [&](const Path& file) {
      engineFiles.push_back(file);
    });
    REQUIRE(engineFiles.size() == 1);
    REQUIRE(engineFiles[0] == Path("/TestEngine/engine.bin"));

    REQUIRE(FileSystem::unmount("TestGame", Path("chMountTestDir/Mod")));
    REQUIRE(FileSystem::fastRead(Path("/TestGame/shared.bin"))[0] == 1);
    REQUIRE(FileSystem::fastRead(Path("/TestGame/sub/deep.bin"))[0] == 7);

    REQUIRE(FileSystem::unmount("TestGame", Path("chMountTestDir/Base")));
    REQUIRE(FileSystem::unmount("TestEngine", Path("chMountTestDir/Engine")));
    REQUIRE_FALSE(FileSystem::isVirtual(Path("/TestGame/shared.bin")));
    REQUIRE(FileSystem::removeAll(root));
  }

  SECTION("disk operations report failures instead of throwing") {
    const Path root("chFileSystemTestDir");
    FileSystem::removeAll(root);

    REQUIRE_FALSE(FileSystem::exists(root));
    REQUIRE(FileSystem::fastRead(Path("chFileSystemTestDir/missing.bin")).empty());
    REQUIRE(FileSystem::openFile(Path("chFileSystemTestDir/missing.bin")) == nullptr);
    REQUIRE_FALSE(FileSystem::removeFile(Path("chFileSystemTestDir/missing.bin")));

    SPtr<DataStream> file = FileSystem::createAndOpenFile(Path("chFileSystemTestDir/a/b/f.bin"));
    REQUIRE(file != nullptr);
    const uint8 bytes[3] = {1, 2, 3};
    file->write(bytes, sizeof(bytes));
    file->close();

    const Vector<uint8> readBack = FileSystem::fastRead(Path("chFileSystemTestDir/a/b/f.bin"));
    REQUIRE(readBack.size() == 3);
    REQUIRE(readBack[2] == 3);

    REQUIRE_FALSE(FileSystem::remove(Path("chFileSystemTestDir/a")));

    Vector<Path> files, directories;
    FileSystem::getChildren(Path("chFileSystemTestDir/a"), files, directories);
    REQUIRE(files.empty());
    REQUIRE(directories.size() == 1);

    int32 visited = 0;
    FileSystem::forEachFileChildRecursive(root, [&](const Path&) { ++visited; });
    REQUIRE(visited == 3);

    REQUIRE(FileSystem::renameFile(Path("chFileSystemTestDir/a/b/f.bin"),
                                   Path("chFileSystemTestDir/a/b/g.bin")));
    REQUIRE(FileSystem::isFile(Path("chFileSystemTestDir/a/b/g.bin")));

    REQUIRE(FileSystem::removeAll(root));
    REQUIRE_FALSE(FileSystem::exists(root));
  }
}

TEST_CASE("chUtilities - CommandLine") {
  SECTION("values and flags") {
    const ANSICHAR* argv[] = {"program", "-option1=value1", "-Option2=Value=2", "-flag"};
    CommandLine::initialize(4, argv);

    REQUIRE(CommandLine::getValue("option1") == "value1");
    REQUIRE(CommandLine::getValue("OPTION2") == "Value=2");
    REQUIRE(CommandLine::hasFlag("FLAG"));
    REQUIRE_FALSE(CommandLine::hasFlag("option1"));
    REQUIRE(CommandLine::getValue("missing", "default") == "default");
    REQUIRE_FALSE(CommandLine::hasFlag("missing"));
    REQUIRE(CommandLine::getArgc() == 4);
  }

  SECTION("dashes and plain arguments") {
    const ANSICHAR* argv[] = {"program", "--double", "plain", "-", "--", "-=x", "-empty="};
    CommandLine::initialize(7, argv);

    REQUIRE(CommandLine::hasFlag("double"));
    REQUIRE_FALSE(CommandLine::hasFlag("plain"));
    REQUIRE_FALSE(CommandLine::hasFlag("lain"));
    REQUIRE_FALSE(CommandLine::hasFlag(""));
    REQUIRE(CommandLine::getValue("", "none") == "none");
    REQUIRE(CommandLine::getValue("empty", "none").empty());
  }

  SECTION("integers") {
    const ANSICHAR* argv[] = {"program", "-Width=1280", "-Height=abc", "-Depth=12px",
                              "-Big=99999999999", "-Negative=-5", "-Empty="};
    CommandLine::initialize(7, argv);

    REQUIRE(CommandLine::getInt("width", 1920) == 1280);
    REQUIRE(CommandLine::getInt("height", 1080) == 1080);
    REQUIRE(CommandLine::getInt("depth", 3) == 3);
    REQUIRE(CommandLine::getInt("big", 7) == 7);
    REQUIRE(CommandLine::getInt("negative") == -5);
    REQUIRE(CommandLine::getInt("empty", 4) == 4);
    REQUIRE(CommandLine::getInt("missing", 9) == 9);
  }

  SECTION("initialize replaces the previous options") {
    const ANSICHAR* first[] = {"program", "-old=1", "-oldFlag"};
    CommandLine::initialize(3, first);
    const ANSICHAR* second[] = {"program"};
    CommandLine::initialize(1, second);

    REQUIRE(CommandLine::getValue("old").empty());
    REQUIRE_FALSE(CommandLine::hasFlag("oldflag"));
  }

  SECTION("tryGetValue tells an empty value from a missing one") {
    const ANSICHAR* argv[] = {"program", "-Empty=", "-Flag"};
    CommandLine::initialize(3, argv);

    REQUIRE(CommandLine::tryGetValue("empty") != nullptr);
    REQUIRE(CommandLine::tryGetValue("empty")->empty());
    REQUIRE(CommandLine::tryGetValue("flag") == nullptr);
    REQUIRE(CommandLine::tryGetValue("missing") == nullptr);
  }
}

TEST_CASE("chUtilities - ConfigFile") {
  SECTION("sections, keys, comments and quotes") {
    ConfigFile file;
    file.parse("Global = 1\r\n"
               "; comment\n"
               "# comment\n"
               "\n"
               "[Window]\n"
               "  Width = 1280  \n"
               "Title=\"  My Game  \"\n"
               "Path=a=b\n"
               "[ Graphics ]\n"
               "API=chDX12\n"
               "[window]\n"
               "WIDTH=1920");

    REQUIRE(*file.getValue("", "global") == "1");
    REQUIRE(*file.getValue("WINDOW", "width") == "1920");
    REQUIRE(*file.getValue("Window", "Title") == "  My Game  ");
    REQUIRE(*file.getValue("Window", "Path") == "a=b");
    REQUIRE(*file.getValue("Graphics", "API") == "chDX12");
    REQUIRE(file.getValue("Window", "Missing") == nullptr);
    REQUIRE(file.getValue("Missing", "Width") == nullptr);
    REQUIRE(file.getSections().size() == 3);
  }

  SECTION("bad lines are skipped") {
    ConfigFile file;
    file.parse("[Broken\nNoEquals\n=NoKey\n[Ok]\nKey=Value", "test.ini");

    REQUIRE(file.getSections().size() == 1);
    REQUIRE(*file.getValue("Ok", "Key") == "Value");
  }

  SECTION("merge replaces equal keys and keeps the rest") {
    ConfigFile base;
    base.parse("[Window]\nWidth=2560\nHeight=1440");
    ConfigFile over;
    over.parse("[window]\nwidth=1280\n[Graphics]\nVSync=true");
    base.merge(over);

    REQUIRE(*base.getValue("Window", "Width") == "1280");
    REQUIRE(*base.getValue("Window", "Height") == "1440");
    REQUIRE(*base.getValue("Graphics", "VSync") == "true");
  }

  SECTION("toText reads back the same entries") {
    ConfigFile file;
    file.setValue("Window", "Title", " spaced ");
    file.setValue("Window", "Width", "1280");
    file.setValue("", "Global", "1");
    file.setValue("Empty", "Value", "");

    const String text = file.toText();
    REQUIRE(text.starts_with("Global=1\n"));

    ConfigFile copy;
    copy.parse(text);
    REQUIRE(*copy.getValue("Window", "Title") == " spaced ");
    REQUIRE(*copy.getValue("Window", "Width") == "1280");
    REQUIRE(*copy.getValue("", "Global") == "1");
    REQUIRE(copy.getValue("Empty", "Value")->empty());
  }

  SECTION("save and load") {
    const Path path = FileSystem::absolutePath(Path("chConfigFileTest/Test.ini"));
    ConfigFile file;
    file.setValue("Window", "Width", "800");
    REQUIRE(file.save(path));

    ConfigFile loaded;
    REQUIRE(loaded.load(path));
    REQUIRE(*loaded.getValue("Window", "Width") == "800");
    REQUIRE_FALSE(loaded.load(path.getDirectory().join(Path("Missing.ini"))));

    REQUIRE(FileSystem::removeAll(path.getDirectory()));
  }
}

TEST_CASE("chUtilities - ConsoleVariable") {
  SECTION("parsing") {
    ConsoleVariable<bool> boolVar("Test.Bool", false, "");
    ConsoleVariable<int32> intVar("Test.Int", 3, "");
    ConsoleVariable<float> floatVar("Test.Float", 1.0f, "");
    ConsoleVariable<String> stringVar("Test.String", "a", "");

    REQUIRE(boolVar.setFromString("ON", ConsoleVariableSource::Console));
    REQUIRE(boolVar.get());
    REQUIRE(boolVar.setFromString(" 0 ", ConsoleVariableSource::Console));
    REQUIRE_FALSE(boolVar.get());
    REQUIRE_FALSE(boolVar.setFromString("maybe", ConsoleVariableSource::Console));
    REQUIRE_FALSE(boolVar.get());

    REQUIRE(intVar.setFromString("-42", ConsoleVariableSource::Console));
    REQUIRE(intVar.get() == -42);
    REQUIRE_FALSE(intVar.setFromString("12px", ConsoleVariableSource::Console));
    REQUIRE_FALSE(intVar.setFromString("", ConsoleVariableSource::Console));
    REQUIRE(intVar.get() == -42);

    REQUIRE(floatVar.setFromString("0.5", ConsoleVariableSource::Console));
    REQUIRE(floatVar.get() == 0.5f);
    REQUIRE(floatVar.toString() == "0.5");

    REQUIRE(stringVar.setFromString("chDX12", ConsoleVariableSource::Console));
    REQUIRE(stringVar.get() == "chDX12");
    REQUIRE(stringVar.getDefault() == "a");
  }

  SECTION("a lower source does not replace a higher one") {
    ConsoleVariable<int32> var("Test.Priority", 1, "");
    REQUIRE(var.getSource() == ConsoleVariableSource::Default);

    REQUIRE(var.setFromString("2", ConsoleVariableSource::CommandLine));
    REQUIRE_FALSE(var.setFromString("3", ConsoleVariableSource::UserConfig));
    REQUIRE(var.get() == 2);
    REQUIRE(var.set(4));
    REQUIRE(var.get() == 4);
    REQUIRE(var.getSource() == ConsoleVariableSource::Code);
  }

  SECTION("find by name or alias, and getAll") {
    ConsoleVariable<int32> var("Test.Find", 1, "", "FindAlias");

    REQUIRE(ConsoleVariables::find("test.find") == &var);
    REQUIRE(ConsoleVariables::find("FINDALIAS") == &var);
    REQUIRE(ConsoleVariables::find("Test.Missing") == nullptr);
    REQUIRE(Algorithm::contains(ConsoleVariables::getAll(), &var));
  }

  SECTION("config values and the command line, before and after registering") {
    const ANSICHAR* argv[] = {"program", "-Test.FromCommandLine=7", "-Alias=8",
                              "-Test.Flag", "-Test.Both=1", "-BothAlias=2"};
    CommandLine::initialize(6, argv);

    ConsoleVariable<int32> early("Test.Early", 0, "");
    ConsoleVariable<int32> fromCommandLine("Test.FromCommandLine", 0, "");
    ConsoleVariable<int32> fromAlias("Test.FromAlias", 0, "", "Alias");
    ConsoleVariable<bool> flag("Test.Flag", false, "");
    ConsoleVariable<int32> both("Test.Both", 0, "", "BothAlias");

    ConsoleVariables::setStartupValue("Test.Early", "1", ConsoleVariableSource::EngineConfig);
    ConsoleVariables::setStartupValue("test.early", "3", ConsoleVariableSource::UserConfig);
    ConsoleVariables::setStartupValue("Test.Early", "2", ConsoleVariableSource::ProjectConfig);
    ConsoleVariables::setStartupValue("Test.FromCommandLine", "5",
                                      ConsoleVariableSource::UserConfig);
    ConsoleVariables::setStartupValue("Test.Late", "9", ConsoleVariableSource::ProjectConfig);
    REQUIRE(early.get() == 0);

    ConsoleVariables::applyStartupValues();
    REQUIRE(early.get() == 3);
    REQUIRE(early.getSource() == ConsoleVariableSource::UserConfig);
    REQUIRE(fromCommandLine.get() == 7);
    REQUIRE(fromAlias.get() == 8);
    REQUIRE(flag.get());
    REQUIRE(both.get() == 1);

    // Registered after the start up, as a plugin's variables are.
    ConsoleVariable<int32> late("Test.Late", 0, "");
    REQUIRE(late.get() == 9);

    ConsoleVariables::setStartupValue("Test.Late", "10", ConsoleVariableSource::UserConfig);
    REQUIRE(late.get() == 10);

    const ANSICHAR* empty[] = {"program"};
    CommandLine::initialize(1, empty);
  }

  SECTION("a destroyed variable leaves the registry") {
    {
      ConsoleVariable<int32> var("Test.Scoped", 1, "");
      REQUIRE(ConsoleVariables::find("Test.Scoped") == &var);
    }
    REQUIRE(ConsoleVariables::find("Test.Scoped") == nullptr);
  }
}

// #endif //CH_PLATFORM_WINDOWS

// int main(int argc, ANSICHAR** argv) {
//   (void)argc;
//   (void)argv;
//   return 0;
// }

TEST_CASE("chUtilities - HashUtils")
{
  // Reference values of 64-bit FNV-1a.
  REQUIRE(HashUtils::hashBytes(nullptr, 0) == HashUtils::FNV_OFFSET_BASIS);
  const uint8 letterA = 'a';
  REQUIRE(HashUtils::hashBytes(&letterA, 1) == 0xAF63DC4C8601EC8Cull);

  const uint64 seed = HashUtils::FNV_OFFSET_BASIS;
  REQUIRE(HashUtils::combine(seed, 7u) == HashUtils::combine(seed, 7u));
  REQUIRE(HashUtils::combine(seed, 7u) != HashUtils::combine(seed, 8u));

  // The order of the fields matters.
  REQUIRE(HashUtils::combine(HashUtils::combine(seed, 1u), 2u) !=
          HashUtils::combine(HashUtils::combine(seed, 2u), 1u));

  // 0.0 and -0.0 compare equal, so they hash the same.
  REQUIRE(HashUtils::combine(seed, 0.0f) == HashUtils::combine(seed, -0.0f));
  REQUIRE(HashUtils::combine(seed, 1.0f) != HashUtils::combine(seed, 2.0f));

  enum class TestEnum : uint8 { First, Second };
  REQUIRE(HashUtils::combine(seed, TestEnum::First) !=
          HashUtils::combine(seed, TestEnum::Second));

  int32 first = 0;
  int32 second = 0;
  REQUIRE(HashUtils::combine(seed, &first) != HashUtils::combine(seed, &second));
}
