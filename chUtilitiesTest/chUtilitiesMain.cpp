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
#ifdef RUN_UNIT_TESTS
#include "chMath.h"
#include "chRadian.h"
#include "chDegree.h"
#include "chVector2.h"
#include "chVector3.h"
#include "chVector4.h"
#include "chMatrix4.h"
#include "chRotator.h"
#include "chQuaternion.h"
#include "chBox2D.h"
#include "chPlane.h"
#include "chSphereBoxBounds.h"
#include "chModule.h"
#include "chDynamicLibManager.h"
#include "chEventSystem.h"
#include "chDebug.h"
#include "chUnicode.h"
#include "chStringUtils.h"
#include "chEventSystem.h"
#include "chPath.h"
#include "chFileSystem.h"
#include "chRandom.h"
#include "chCommandParser.h"


#define CATCH_CONFIG_MAIN
#include "catch.hpp"


using namespace chEngineSDK;

/************************************************************************/
/*
 * Basic type sizes.
 */
 /************************************************************************/
TEST_CASE("chUtilities - BasicTypeSize") {
    REQUIRE(sizeof(unsigned char) == 1);
    REQUIRE(sizeof(uint8) == 1);
    REQUIRE(sizeof(uint16) == 2);
    REQUIRE(sizeof(uint32) == 4);
    REQUIRE(sizeof(uint64) == 8);

    REQUIRE(sizeof(int8) == 1);
    REQUIRE(sizeof(int16) == 2);
    REQUIRE(sizeof(int32) == 4);
    REQUIRE(sizeof(int64) == 8);

    REQUIRE(sizeof(char) == 1);
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
    REQUIRE(Math::FORTH_PI == Approx(0.78539816339f));

    // Functions
    REQUIRE(Math::unwindDegrees(270.0f) == Approx(-90.0f));
    REQUIRE(Math::unwindRadians(4.71239f) == Approx(-1.5707955f));

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
    REQUIRE(DegreeDefault.valueDegree() != 0.0f);

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

    //Degree to Degree
    REQUIRE(DegreeToCompare1 < DegreeToCompare2);
    REQUIRE_FALSE(DegreeToCompare1 > DegreeToCompare2);
    REQUIRE(DegreeToCompare1 <= DegreeToCompare1);
    REQUIRE_FALSE(DegreeToCompare1 >= DegreeToCompare2);
    REQUIRE_FALSE(DegreeToCompare1 == DegreeToCompare2);
    REQUIRE(DegreeToCompare1 != DegreeToCompare2);

    //Degree to Radian
    REQUIRE(DegreeToCompare1 < RadianToCompare1);
    REQUIRE_FALSE(DegreeToCompare1 > RadianToCompare1);
    REQUIRE(DegreeToCompare1 <= RadianToCompare1);
    REQUIRE_FALSE(DegreeToCompare1 >= RadianToCompare1);
    REQUIRE_FALSE(DegreeToCompare1 == RadianToCompare1);
    REQUIRE(DegreeToCompare1 != RadianToCompare1);

    //Degree to float
    REQUIRE(DegreeToCompare1 < FloatToCompare1);
    REQUIRE_FALSE(DegreeToCompare1 > FloatToCompare1);
    REQUIRE(DegreeToCompare1 <= FloatToCompare1);
    REQUIRE_FALSE(DegreeToCompare1 >= FloatToCompare1);
    REQUIRE_FALSE(DegreeToCompare1 == FloatToCompare1);
    REQUIRE(DegreeToCompare1 != FloatToCompare1);

    //Float as lValue to Degree
    REQUIRE(FloatToCompare2 < DegreeToCompare2);
    REQUIRE_FALSE(FloatToCompare2 > DegreeToCompare2);
    REQUIRE(FloatToCompare2 <= DegreeToCompare2);
    REQUIRE_FALSE(FloatToCompare2 >= DegreeToCompare2);
    REQUIRE_FALSE(FloatToCompare2 == DegreeToCompare2);
    REQUIRE(FloatToCompare2 != DegreeToCompare2);

    //Float as rValue to Degree
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

    //Radian to Radian
    REQUIRE(RadianToCompare1 < RadianToCompare2);
    REQUIRE_FALSE(RadianToCompare1 > RadianToCompare2);
    REQUIRE(RadianToCompare1 <= RadianToCompare1);
    REQUIRE_FALSE(RadianToCompare1 >= RadianToCompare2);
    REQUIRE_FALSE(RadianToCompare1 == RadianToCompare2);
    REQUIRE(RadianToCompare1 != RadianToCompare2);

    //Radian to Degree
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

    REQUIRE(Math::cosh(RadianToTest2) == Approx(11.59195328f).margin(Math::KINDA_SMALL_NUMBER));
    REQUIRE(Math::cosh(DegreeToTest2) == Approx(11.59195328f).margin(Math::KINDA_SMALL_NUMBER));

    REQUIRE(Math::sinh(RadianToTest2) == Approx(11.548739368f).margin(Math::KINDA_SMALL_NUMBER));
    REQUIRE(Math::sinh(DegreeToTest2) == Approx(11.548739368f).margin(Math::KINDA_SMALL_NUMBER));

    REQUIRE(Math::tanh(RadianToTest1) == Approx(0.91715234f).margin(Math::KINDA_SMALL_NUMBER));
    REQUIRE(Math::tanh(DegreeToTest1) == Approx(0.91715234f).margin(Math::KINDA_SMALL_NUMBER));

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
    REQUIRE(RadiancoAtan2.valueRadian() == Approx(0.785398163397f).margin(Math::KINDA_SMALL_NUMBER));
    REQUIRE(DegreecoAtan2.valueRadian() == Approx(0.785398163397f).margin(Math::KINDA_SMALL_NUMBER));

    Radian RadiancoAcosh = Math::acosh(2.0f);
    Degree DegreecoAcosh(Math::acosh(2.0f));
    REQUIRE(RadiancoAcosh.valueRadian() == Approx(1.316957896925f).margin(Math::KINDA_SMALL_NUMBER));
    REQUIRE(DegreecoAcosh.valueRadian() == Approx(1.316957896925f).margin(Math::KINDA_SMALL_NUMBER));

    Radian RadiancoAsinh = Math::asinh(2.0f);
    Degree DegreecoAsinh;
    DegreecoAsinh = Math::asinh(2.0f);
    REQUIRE(RadiancoAsinh.valueRadian() == Approx(1.443635475179f).margin(Math::KINDA_SMALL_NUMBER));
    REQUIRE(DegreecoAsinh.valueRadian() == Approx(1.443635475179f).margin(Math::KINDA_SMALL_NUMBER));

    Radian RadiancoAtanh = Math::atanh(0.6f);
    Degree DegreecoAtanh;
    DegreecoAtanh = Math::atanh(0.6f);
    REQUIRE(RadiancoAtanh.valueRadian() == Approx(0.69314718056f).margin(Math::KINDA_SMALL_NUMBER));
    REQUIRE(DegreecoAtanh.valueRadian() == Approx(0.69314718056f).margin(Math::KINDA_SMALL_NUMBER));
}


/************************************************************************/
/*
 * Vectors
 */
 /************************************************************************/
TEST_CASE("chUtilities - Vector2") {
    REQUIRE(sizeof(Vector2) == 8);

    Vector2 VectorDefault;
    const Vector2 VectorZero = Vector2::ZERO;
    REQUIRE(VectorDefault != VectorZero);

    std::cout << "Print Vector Test: " << VectorZero;

    REQUIRE(VectorZero.x == Approx(0.0f));
    REQUIRE(VectorZero.y == Approx(0.0f));

    Array<float, 2>vec2Z = {0.0f, 0.0f};
    REQUIRE(VectorZero.xy() == vec2Z);
    REQUIRE_FALSE(VectorZero.xy() == VectorDefault.xy());

    REQUIRE(VectorZero.xy()[0] == VectorZero.x);
    REQUIRE(VectorZero.xy()[1] == VectorZero.y);

    Vector2 VectorComponents(15.0f, 10.0f);
    VectorComponents.x = 19.0f;
    VectorComponents.y = 19.5f;

    REQUIRE(VectorComponents.x == Approx(19.0f));
    REQUIRE(VectorComponents.y == Approx(19.5f));

    float v2[2] = {1.3f, 2.7f};
    VectorComponents = v2;
    REQUIRE(VectorComponents.x == Approx(1.3f));
    REQUIRE(VectorComponents.y == Approx(2.7f));

    const Vector2 VectorFromFloat2(1.0f, 1.0f);
    const Vector2 VectorUnity = Vector2::UNIT;
    REQUIRE(VectorFromFloat2 == VectorUnity);

    const Vector2 VectorFromFloat1X(1.0f, 0.0f);
    const Vector2 VectorUnity_X = Vector2::UNIT_X;
    REQUIRE(VectorFromFloat1X == VectorUnity_X);

    const Vector2 VectorFromFloat1Y(0.0f, 1.0f);
    const Vector2 VectorUnity_Y = Vector2::UNIT_Y;
    REQUIRE(VectorFromFloat1Y == VectorUnity_Y);

    float ScalarCross = VectorFromFloat1X.cross(VectorUnity);
    REQUIRE(ScalarCross == Approx(1.0f));

    const Vector2 NegativeY = Vector2(0.0f, -1.0f);

    float ScalatDot = VectorFromFloat1Y.dot(VectorFromFloat1X);
    REQUIRE(ScalatDot == Approx(0.0f));

    Vector2 VectorToNormalize(15.0f, 0.0f);
    Vector2 VectorNormalized = VectorToNormalize.getNormalized();
    float Magnitude = VectorToNormalize.magnitude();
    float SqrtMagnitude = VectorToNormalize.sqrMagnitude();
    VectorToNormalize.normalize();
    REQUIRE(VectorToNormalize == VectorNormalized);
    REQUIRE(VectorToNormalize.magnitude() == Approx(1.0f));
    REQUIRE(Magnitude == Approx(15.0f));
    REQUIRE(SqrtMagnitude == Approx(225.0f));

    Vector2 VectorToProjectA(6.0f, 5.0f);
    Vector2 VectorToProjectB(10.0f, 3.0f);
    Vector2 VectorToProjectC(5.0f, 1.0f);
    Vector2 ProjectionAB = VectorToProjectA.projection(VectorToProjectB);
    REQUIRE(ProjectionAB == Vector2(6.88073397f, 2.06422019f));

    const Vector2 VectorMultScalar = Vector2::ZERO * 5;
    Vector2 VectorMultScalarThis = Vector2::ZERO;
    VectorMultScalarThis *= 5.0f;

    REQUIRE(VectorMultScalarThis == VectorMultScalar);

    float ArrayToV2[2] = {1.0f, 20.0f};
    Vector2 V2FromArrayConstructor(ArrayToV2);
    Vector2 V2ToCheckArray(ArrayToV2[0], ArrayToV2[1]);
    REQUIRE(V2FromArrayConstructor == V2ToCheckArray);

    Vector2 V2Add(1.0f, 2.0f);
    const Vector2 V2ToAdd(1.0f, 2.0f);
    const Vector2 AddedVector = V2Add + V2ToAdd;
    V2Add += V2ToAdd;
    REQUIRE(V2Add == AddedVector);

    Vector2 V2Subtract(1.0f, 2.0f);
    const Vector2 V2ToSubtract(1.0f, 2.0f);
    const Vector2 SubtractedVector = V2Subtract - V2ToSubtract;
    V2Subtract -= V2ToSubtract;
    REQUIRE(V2Subtract == SubtractedVector);
}

TEST_CASE("chUtilities - Vector3") {
    REQUIRE(sizeof(Vector3) == 12);

    const Vector2 vector2 = Vector2::ZERO;
    Vector3 V3FromV2(vector2);
    REQUIRE(V3FromV2 == Vector3::ZERO);

    Vector3 VectorDefault;
    const Vector3 VectorZero = Vector3::ZERO;
    REQUIRE(VectorDefault != VectorZero);

    std::cout << "Print Vector Test: " << VectorZero;

    REQUIRE(VectorZero.x == Approx(0.0f));
    REQUIRE(VectorZero.y == Approx(0.0f));
    REQUIRE(VectorZero.z == Approx(0.0f));

    Array<float,3> vec3Z = {0.0f, 0.0f, 0.0f};
    Array<float,2> vec2Z = {0.0f, 0.0f};
    REQUIRE(VectorZero.xyz() == vec3Z);
    REQUIRE(VectorZero.xy() == vec2Z);

    REQUIRE(VectorZero.xyz()[0] == Approx(VectorZero.x));
    REQUIRE(VectorZero.xy ()[0] == Approx(VectorZero.x));
    REQUIRE(VectorZero.xyz()[1] == Approx(VectorZero.y));
    REQUIRE(VectorZero.xy ()[1] == Approx(VectorZero.y));
    REQUIRE(VectorZero.xyz()[2] == Approx(VectorZero.z));
    REQUIRE(VectorZero.xy() != VectorDefault.xy());

    REQUIRE(VectorZero.xy()[0] == VectorZero.x);
    REQUIRE(VectorZero.xy()[1] == VectorZero.y);

    REQUIRE(VectorZero.xyz()[0] == VectorZero.x);
    REQUIRE(VectorZero.xyz()[1] == VectorZero.y);
    REQUIRE(VectorZero.xyz()[2] == VectorZero.z);

    Vector3 VectorComponents(15.0f, 10.0f, 22.0f);
    VectorComponents.x = 19.0f;
    VectorComponents.y = 19.5f;
    VectorComponents.xyz()[2] = 22.0f;

    REQUIRE(VectorComponents.x == Approx(19.0f));
    REQUIRE(VectorComponents.y == Approx(19.5f));
    REQUIRE(VectorComponents.xyz()[2] == Approx(22.0f));

    const Vector3 VectorFromFloat2(1.0f, 1.0f, 1.0f);
    const Vector3 VectorUnity = Vector3::UNIT;
    REQUIRE(VectorFromFloat2 == VectorUnity);

    const Vector3 VectorFromFloat1X(0.0f, 1.0f, 0.0f);
    const Vector3 VectorUnity_X = Vector3::RIGHT;
    REQUIRE(VectorFromFloat1X == VectorUnity_X);

    const Vector3 VectorFromFloat1Y(0.0f, 0.0f, 1.0f);
    const Vector3 VectorUnity_Y = Vector3::UP;
    REQUIRE(VectorFromFloat1Y == VectorUnity_Y);

    const Vector3 VectorFromFloat1Z(1.0f, 0.0f, 0.0f);
    const Vector3 VectorUnity_Z = Vector3::FORWARD;
    REQUIRE(VectorFromFloat1Z == VectorUnity_Z);

    Vector3 Vector3Crossed = VectorFromFloat1X.cross(VectorUnity_Y);
    REQUIRE(Vector3Crossed == Vector3::FORWARD);

    float ScalatDot = VectorFromFloat1Y.dot(VectorFromFloat1X);
    REQUIRE(ScalatDot == Approx(0.0f));

    Vector3 VectorToNormalize(15.0f, 0.0f, 0.0f);
    Vector3 VectorNormalized = VectorToNormalize.getNormalized();
    float Magnitude = VectorToNormalize.magnitude();
    float SqrtMagnitude = VectorToNormalize.sqrMagnitude();
    VectorToNormalize.normalize();
    REQUIRE(VectorToNormalize == VectorNormalized);
    REQUIRE(VectorToNormalize.magnitude() == Approx(1.0f));
    REQUIRE(Magnitude == Approx(15.0f));
    REQUIRE(SqrtMagnitude == Approx(225.0f));

    Vector3 VectorToProjectA(6.0f, 5.0f, 0.0f);
    Vector3 VectorToProjectB(10.0f, 3.0f, 0.0f);
    Vector3 VectorToProjectC(5.0f, 1.0f, 0.0f);
    Vector3 ProjectionAB = VectorToProjectA.projection(VectorToProjectB);
    REQUIRE(ProjectionAB == Vector3(6.880733944954128f, 2.064220183486239f, 0.0f));

    const Vector3 VectorMultScalar = Vector3::UP * 5;
    Vector3 VectorMultScalarThis = Vector3::UP;
    VectorMultScalarThis *= 5.0f;

    REQUIRE(VectorMultScalarThis == VectorMultScalar);

    float ArrayToV3[3] = {1.0f, 20.0f, 0.0f};
    Vector3 V2FromArrayConstructor(ArrayToV3);
    Vector3 V2ToCheckArray(ArrayToV3[0], ArrayToV3[1], ArrayToV3[2]);
    REQUIRE(V2FromArrayConstructor == V2ToCheckArray);

    Vector3 V3Add(1.0f, 2.0f, 3.0f);
    const Vector3 V3ToAdd(1.0f, 2.0f, 3.0f);
    const Vector3 AddedVector = V3Add + V3ToAdd;
    V3Add += V3ToAdd;
    REQUIRE(V3Add == AddedVector);

    Vector3 V3Subtract(1.0f, 2.0f, 3.0f);
    const Vector3 V3ToSubtract(1.0f, 2.0f, 3.0f);
    const Vector3 SubtractedVector = V3Subtract - V3ToSubtract;
    V3Subtract -= V3ToSubtract;
    REQUIRE(V3Subtract == SubtractedVector);
}

TEST_CASE("chUtilities - Vector4") {
    REQUIRE(sizeof(Vector4) == 16);

    Vector4 VectorDefault;
    const Vector4 VectorZero = Vector4::ZERO;
    REQUIRE(VectorDefault != VectorZero);

    std::cout << "Print Vector Test: " << VectorZero;

    REQUIRE(VectorZero.x == Approx(0.0f));
    REQUIRE(VectorZero.y == Approx(0.0f));
    REQUIRE(VectorZero.z == Approx(0.0f));

    Array<float, 3> vec3Z = {0.0f, 0.0f, 0.0f};
    Array<float, 2> vec2Z = {0.0f, 0.0f};
    REQUIRE(VectorZero.xyz() == vec3Z);
    REQUIRE(VectorZero.xy() == vec2Z);

    REQUIRE(VectorZero.xyz()[0] == Approx(VectorZero.x));
    REQUIRE(VectorZero.xy()[0] == Approx(VectorZero.x));
    REQUIRE(VectorZero.xyz()[1] == Approx(VectorZero.y));
    REQUIRE(VectorZero.xy()[1] == Approx(VectorZero.y));
    REQUIRE(VectorZero.xyz()[2] == Approx(VectorZero.z));
    REQUIRE_FALSE(VectorZero.xy() == VectorDefault.xy());

    REQUIRE(VectorZero.xy()[0] == VectorZero.x);
    REQUIRE(VectorZero.xy()[1] == VectorZero.y);

    REQUIRE(VectorZero.xyz()[0] == VectorZero.x);
    REQUIRE(VectorZero.xyz()[1] == VectorZero.y);
    REQUIRE(VectorZero.xyz()[2] == VectorZero.z);

    Vector4 VectorComponents(15.0f, 10.0f, 22.0f);
    VectorComponents.x = 19.0f;
    VectorComponents.y = 19.5f;
    VectorComponents.z = 22.0f;

    REQUIRE(VectorComponents.x == Approx(19.0f));
    REQUIRE(VectorComponents.y == Approx(19.5f));
    REQUIRE(VectorComponents.xyz()[2] == Approx(22.0f));

    const Vector4 VectorFromFloat2(1.0f, 1.0f, 1.0f);
    const Vector4 VectorUnity = Vector4::UNIT;
    REQUIRE(VectorFromFloat2 == VectorUnity);

    const Vector4 VectorFromFloat1X(1.0f, 0.0f, 0.0f);
    const Vector4 VectorUnity_X = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
    REQUIRE(VectorFromFloat1X == VectorUnity_X);

    const Vector4 VectorFromFloat1Y(0.0f, 1.0f, 0.0f);
    const Vector4 VectorUnity_Y = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
    REQUIRE(VectorFromFloat1Y == VectorUnity_Y);

    const Vector4 VectorFromFloat1Z(0.0f, 0.0f, 1.0f);
    const Vector4 VectorUnity_Z = Vector4(0.0f, 0.0f, 1.0f, 1.0f);
    REQUIRE(VectorFromFloat1Z == VectorUnity_Z);

    Vector4 Vector4Crossed = VectorFromFloat1X.cross(VectorUnity_Y);
    Vector4 Totest(VectorUnity_Z);
    Totest.w = 0.0f;
    REQUIRE(Vector4Crossed == Totest);

    float ScalatDot = VectorFromFloat1Y.dot(VectorFromFloat1X);
    REQUIRE(ScalatDot == Approx(1.0f));

    Vector4 VectorToNormalize(15.0f, 0.0f, 0.0f);
    Vector4 VectorNormalized = VectorToNormalize.getNormalized();
    float Magnitude = VectorToNormalize.magnitude();
    float SqrtMagnitude = VectorToNormalize.sqrMagnitude();
    VectorToNormalize.normalize();
    REQUIRE(VectorToNormalize == VectorNormalized);
    REQUIRE(VectorToNormalize.magnitude() == Approx(1.0f));
    REQUIRE(Magnitude == Approx(15.0332963f));
    REQUIRE(SqrtMagnitude == Approx(226.0f));

    const Vector4 VectorMultScalar = Vector4(0.0f, 1.0f, 0.0f, 1.0f) * 5;
    Vector4 VectorMultScalarThis = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
    VectorMultScalarThis *= 5.0f;

    REQUIRE(VectorMultScalarThis == VectorMultScalar);

    float ArrayToV4[4] = {1.0f, 20.0f, 0.0f, 1.0f};
    Vector4 V2FromArrayConstructor(ArrayToV4);
    Vector4 V2ToCheckArray(ArrayToV4[0], ArrayToV4[1], ArrayToV4[2], ArrayToV4[3]);
    REQUIRE(V2FromArrayConstructor == V2ToCheckArray);

    Vector4 V4Add(1.0f, 2.0f, 3.0f);
    const Vector4 V4ToAdd(1.0f, 2.0f, 3.0f);
    const Vector4 AddedVector = V4Add + V4ToAdd;
    V4Add += V4ToAdd;
    REQUIRE(V4Add == AddedVector);

    Vector4 V4Subtract(1.0f, 2.0f, 3.0f);
    const Vector4 V4ToSubtract(1.0f, 2.0f, 3.0f);
    const Vector4 SubtractedVector = V4Subtract - V4ToSubtract;
    V4Subtract -= V4ToSubtract;
    REQUIRE(V4Subtract == SubtractedVector);
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
TEST_CASE("chUtilities - Matrix4") {
    REQUIRE(sizeof(Matrix4) == 16 * 4);

    const Matrix4 Identity(1.0f, 0.0f, 0.0f, 0.0f,
                           0.0f, 1.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 1.0f, 0.0f,
                           0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << Identity;

    REQUIRE(Identity == Matrix4::IDENTITY);

    Matrix4 Temporal1(Identity);
    REQUIRE(Identity == Temporal1);

    Matrix4 Temporal2(9.0f, 0.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 0.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f);

    Temporal1.m00 = 9.0f;
    REQUIRE(Temporal1 == Temporal2);

    Matrix4 MultiplicationResult = Temporal1 * Matrix4::UNITY;

    const Matrix4 ActualResult(9.0f, 9.0f, 9.0f, 9.0f,
                               1.0f, 1.0f, 1.0f, 1.0f,
                               1.0f, 1.0f, 1.0f, 1.0f,
                               1.0f, 1.0f, 1.0f, 1.0f);

    REQUIRE(MultiplicationResult == ActualResult);

    Temporal1 *= Matrix4::UNITY;
    REQUIRE(Temporal1 == ActualResult);

    Matrix4 AdditionResult = Temporal1 + ActualResult;
    const Matrix4 RealAdditionResult(18.0f, 18.0f, 18.0f, 18.0f,
                                     2.0f, 2.0f, 2.0f, 2.0f,
                                     2.0f, 2.0f, 2.0f, 2.0f,
                                     2.0f, 2.0f, 2.0f, 2.0f);
    REQUIRE(AdditionResult == RealAdditionResult);

    Matrix4 SubtractResult = AdditionResult - ActualResult;
    REQUIRE(SubtractResult == ActualResult);

    Matrix4 MultiplicationValueResult = Matrix4::IDENTITY * 4.0f;
    const Matrix4 ValMulFixedresult(4.0f, 0.0f, 0.0f, 0.0f,
                                    0.0f, 4.0f, 0.0f, 0.0f,
                                    0.0f, 0.0f, 4.0f, 0.0f,
                                    0.0f, 0.0f, 0.0f, 4.0f);
    REQUIRE(MultiplicationValueResult == ValMulFixedresult);

    Temporal2 *= 0.0f;
    REQUIRE(Temporal2 == Matrix4::ZERO);

    Temporal1.setIdentity();
    REQUIRE(Temporal1 == Matrix4::IDENTITY);

    Temporal2 = Matrix4::IDENTITY;
    REQUIRE(Temporal2 == Matrix4::IDENTITY);

    Temporal2 = RealAdditionResult.getTransposed();
    const Matrix4 TransposedResult(18.0f, 2.0f, 2.0f, 2.0f,
                                   18.0f, 2.0f, 2.0f, 2.0f,
                                   18.0f, 2.0f, 2.0f, 2.0f,
                                   18.0f, 2.0f, 2.0f, 2.0f);
    REQUIRE(Temporal2 == TransposedResult);

    Temporal2.transposed();
    REQUIRE(Temporal2 == RealAdditionResult);

    float Determinant = Temporal2.getDeterminant();
    REQUIRE(Determinant == Approx(0.0f));

    Matrix4 NotValidInverse = Temporal2.getInverse();
    REQUIRE(NotValidInverse == Matrix4::IDENTITY);

    Matrix4 ToInverse(1.0f, 1.0f, 1.0f, 0.0f,
                      0.0f, 3.0f, 1.0f, 2.0f,
                      2.0f, 3.0f, 1.0f, 0.0f,
                      1.0f, 0.0f, 2.0f, 1.0f);
    const Matrix4 ValidInverse = ToInverse.getInverse();

    REQUIRE(ValidInverse == Matrix4(-3.0f, -0.5f, 1.5f, 1.0f,
                                    1.0f, 0.25f, -0.25f, -0.5f,
                                    3.0f, 0.25f, -1.25f, -0.5f,
                                    -3.0f, 0.0f, 1.0f, 1.0f));

    Vector4 FRow(ToInverse.FirstRow);
    REQUIRE(FRow == Vector4(1.0f, 1.0f, 1.0f, 0.0f));

    Vector4 SecRow(ToInverse.SecondRow);
    REQUIRE(Vector4(0.0f, 3.0f, 1.0f, 2.0f) == SecRow);

    Vector4 ThirdRow(ToInverse.ThirdRow);
    REQUIRE(ThirdRow == Vector4(2.0f, 3.0f, 1.0f, 0.0f));

    Vector4 FourthRow(ToInverse.FourthRow);
    REQUIRE(FourthRow == Vector4(1.0f, 0.0f, 2.0f, 1.0f));

    const TranslationMatrix T(Vector3(10.0f, 2.0f, 1.0f));
    Vector4 newPos = T.transformPosition(Vector3(1.8f, 52.f, 26.6f));
    REQUIRE(newPos == Vector4(11.8f, 54.f, 27.6f, 1.0f));

    /************************************************************************/
    /*
     * Matrix inheritance.
     */
    /************************************************************************/

    REQUIRE(sizeof(TranslationMatrix) == 16 * 4);

    const TranslationMatrix PositionMat(Vector3(2.0f, 3.0f, 150.0f));
    REQUIRE(Vector4(2.0f, 3.0f, 150.0f, 1.0f) == PositionMat.FourthRow);

    Rotator Rotator4Matrix(90.f, 180.f, 90.f);
    const RotationTranslationMatrix RotTransMatrix(Rotator4Matrix, Vector3::UNIT);

    float SP = Math::sin(Rotator4Matrix.pitch);
    float CP = Math::cos(Rotator4Matrix.pitch);

    float SY = Math::sin(Rotator4Matrix.yaw);
    float CY = Math::cos(Rotator4Matrix.yaw);

    float SR = Math::sin(Rotator4Matrix.roll);
    float CR = Math::cos(Rotator4Matrix.roll);

    REQUIRE(Vector4(CP * CY, CP * SY, SP, 0.0f) == RotTransMatrix.FirstRow);
    REQUIRE(Vector4(SR * SP * CY - CR * SY,
                    SR * SP * SY + CR * CY,
                    -SR * CP, 0.0f) == RotTransMatrix.SecondRow);
    REQUIRE(Vector4(-(CR * SP * CY + SR * SY),
                    CY * SR - CR * SP * SY,
                    CR * CP, 0.0f) == RotTransMatrix.ThirdRow);
    REQUIRE(Vector4(1.0f, 1.0f, 1.0f, 1.0f) == RotTransMatrix.FourthRow);

    RotationMatrix RM(Rotator4Matrix);
    REQUIRE(Vector4(CP * CY, CP * SY, SP, 0.0f) == RM.FirstRow);
    REQUIRE(Vector4(SR * SP * CY - CR * SY,
                    SR * SP * SY + CR * CY,
                    -SR * CP, 0.0f) == RM.SecondRow);
    REQUIRE(Vector4(-(CR * SP * CY + SR * SY),
                    CY * SR - CR * SP * SY,
                    CR * CP, 0.0f) == RM.ThirdRow);
    REQUIRE(Vector4(0.0f, 0.0f, 0.0f, 1.0f) == RM.FourthRow);

    ScaleRotationTranslationMatrix SRTM(Vector3(2.f, 2.f, 2.f),
                                        Rotator4Matrix,
                                        Vector3::ZERO);

    REQUIRE(Vector4((CP * CY) * 2, (CP * SY) * 2, SP * 2, 0.0f) == SRTM.FirstRow);
    REQUIRE(Vector4((SR * SP * CY - CR * SY) * 2,
                    (SR * SP * SY + CR * CY) * 2,
                    (-SR * CP) * 2, 0.0f) == SRTM.SecondRow);
    REQUIRE(Vector4(-(CR * SP * CY + SR * SY) * 2,
                    (CY * SR - CR * SP * SY) * 2,
                    (CR * CP) * 2, 0.0f) == SRTM.ThirdRow);
    REQUIRE(Vector4(0.0f, 0.0f, 0.0f, 1.0f) == SRTM.FourthRow);

    /************************************************************************/
    /*
     * Quaternion Matrix
     */
    /************************************************************************/
    const Matrix4 M270X90Y(0.0f, 0.0f, 1.0f, 0.0f,
                           -1.0f, -0.0f, 0.0f, 0.0f,
                           0.0f, -1.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 0.0f, 1.0f);
    Quaternion QFromMatrix = M270X90Y.toQuaternion();
    const Quaternion RIGHT_QUATERNION(0.5f, -0.5f, 0.5f, -0.5f);
    REQUIRE(RIGHT_QUATERNION.nearEqual(QFromMatrix));
}


/************************************************************************/
/*
 * Quaternion.
 */
 /************************************************************************/
TEST_CASE("chUtilities - Quaternion") {
    REQUIRE(sizeof(Quaternion) == 4 * 4);

    Quaternion quaternionDefault;
    const Quaternion RealQuat90Pitch(0.0f, 0.707106769f, 0.0f, 0.707106769f);
    const Quaternion RealQuat90Yaw(0.0f, 0.0f, 0.707106769f, 0.707106769f);
    const Quaternion RealQuat90Roll(0.707106769f, 0.0f, 0.0f, 0.707106769f);

    REQUIRE_FALSE(quaternionDefault == RealQuat90Roll);

    Rotator rotPitch90(90.0f, 0.0f, 0.0f);
    Rotator rotYaw90(0.0f, 90.0f, 0.0f);
    const Rotator rotRoll90(0.0f, 0.0f, 90.0f);

    const Quaternion quatYaw90(rotYaw90);
    const Quaternion quatRoll90(rotRoll90);
    const Quaternion quatPitch90(rotPitch90);

    REQUIRE(quatRoll90 == RealQuat90Roll);
    REQUIRE(quatYaw90 == RealQuat90Yaw);
    REQUIRE(quatPitch90 == RealQuat90Pitch);

    Rotator RotFromQuat90Roll = quatRoll90.toRotator();
    Rotator RotFromQuat90Yaw = quatYaw90.toRotator();
    Rotator RotFromQuat90Pitch = quatPitch90.toRotator();

    REQUIRE(RotFromQuat90Roll.nearEqual(rotRoll90));
    REQUIRE(RotFromQuat90Yaw.nearEqual(rotYaw90));
    REQUIRE(RotFromQuat90Pitch.nearEqual(rotPitch90));

    const Rotator RotatorXY90(90.0f, 90.0f, 0.0f);
    Quaternion QXY90(RotatorXY90);
    const Quaternion QRot90XYHardCoded(-0.5f, 0.5f, 0.5f, 0.5f);
    REQUIRE(QXY90.nearEqual(QRot90XYHardCoded));

    const Rotator RotatorX270Y90(35.0f, 45.0f, 0.0f);
    Quaternion QX270Y90(RotatorX270Y90);
    const Quaternion QRotX270Y90HardCoded(-0.115075134f, 0.277815908f, 0.364971697f, 0.881119549f);
    REQUIRE(QX270Y90.nearEqual(QRotX270Y90HardCoded));

    const Rotator TestRotator(65.0f, 33.0f, 20.0f);
    Quaternion Q65P33Y120R(TestRotator);
    const Quaternion QRotHD6533120(-0.0098606f, 0.5489418f, 0.1464381f, 0.8228739f);
    REQUIRE(Q65P33Y120R.nearEqual(QRotHD6533120));

    QXY90.conjugate();
    Quaternion QConjugated = QRot90XYHardCoded.getConjugated();
    REQUIRE(QXY90.nearEqual(QConjugated));

    QX270Y90.normalize();
    Quaternion QNormalized = QRotX270Y90HardCoded.getNormalized();
    REQUIRE(QX270Y90.nearEqual(QNormalized));

    Quaternion Qnan((float)NAN, 0.0f, 0.0f, (float)NAN);
    // REQUIRE(Qnan.containsNaN()); //Quaternion fixes itself when it is in debug mode.

    const Vector3 Right = quatYaw90.rotateVector(Vector3::FORWARD);
    REQUIRE(Right.nearEqual(Vector3::RIGHT, Math::KINDA_SMALL_NUMBER));

    const Vector3 Backwards = quatYaw90.rotateVector(Right);
    REQUIRE(Backwards.nearEqual(-Vector3::FORWARD, Math::KINDA_SMALL_NUMBER));

    Quaternion Q45PY(Rotator(45.0f, 45.0f, 0.0f));
    const Vector3 VMiddle(.5f, .5f, -0.707106829f);
    Vector3 TestMiddle = Q45PY.rotateVector(Vector3::FORWARD);
    REQUIRE(TestMiddle.nearEqual(VMiddle));
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

    const TranslationMatrix T(Vector3(10.0f, 2.0f, 1.0f));
    AABox TenAAB = UnitBox.transformProjection(T);
    REQUIRE(TenAAB.maxPoint == Vector3(11.f, 3.f, 2.f));

    const Vector3 Half(.5f, .5f, .5f);
    REQUIRE(Math::pointAABIntersection(Half, UnitBox));
    REQUIRE_FALSE(Math::pointAABIntersection(Vector3::UNIT * 2, UnitBox));

    const AABox Box2(Vector3::UNIT, Vector3::UNIT * 2);
    const AABox FarBox(Vector3::UNIT * 3.1f, Vector3::UNIT * 6);
    REQUIRE(Math::aabAABIntersection(Box2, Movable));
    REQUIRE_FALSE(Math::aabAABIntersection(Movable, FarBox));

    const Vector<Vector3> ArrayPoints = {
        {-1.0f, -1.0f, 1.0f}, {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
        {7.0f, 12.0f, 22.0f}, {4.0f, 2.0f, 1.0f}, {6.0f, 3.1f, 22.6f}
    };

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
    REQUIRE(Math::aabPlaneIntersection(Aabox, Plane2AABoxTrue));
    REQUIRE_FALSE(Math::aabPlaneIntersection(Aabox, Plane2AABoxFalse));
}

TEST_CASE("chUtilities - Sphere") {
    REQUIRE(sizeof(Sphere) == 16);

    const Vector<Vector3> ArrayPoints = {
        {-100.0f, -1.0f, 1.0f}, {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
        {7.0f, 10.0f, 22.0f}, {4.0f, 2.0f, 1.0f}, {6.0f, 3.1f, 22.6f},
        {-99.0f, -1.0f, 1.0f}, {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
        {7.0f, 10.0f, 22.0f}, {4.0f, 2.0f, 1.0f}, {6.0f, 3.1f, 22.6f}
    };

    const Sphere FromPoints(ArrayPoints);
    REQUIRE(FromPoints.radius == Approx(55.0949364f));
    REQUIRE(FromPoints.center == Vector3(-46.5000000f, 4.5f, 10.30000002f));

    const Sphere Center2(Vector3::UNIT * 3, 1);
    REQUIRE_FALSE(Math::pointSphereIntersect(Vector3::ZERO, Center2));
    REQUIRE(Math::pointSphereIntersect(Vector3::UNIT * 2.5, Center2));

    const Sphere Center3(Vector3::UNIT * 3, 2);
    const Sphere Center1(Vector3::UNIT, .5f);
    REQUIRE(Math::sphereSphereIntersect(Center2, Center3));
    REQUIRE_FALSE(Math::sphereSphereIntersect(Center3, Center1));

    const AABox Aabox(Vector3::ZERO, Vector3::UNIT);
    REQUIRE(Math::aabSphereintersection(Aabox, Center1));
    REQUIRE_FALSE(Math::aabSphereintersection(Aabox, Center3));
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
        {7.0f, 10.0f, 22.0f}, {4.0f, 2.0f, 1.0f}, {6.0f, 3.1f, 22.6f},
        {-99.0f, -1.0f, 1.0f}, {7.0f, 8.0f, -2.0f}, {1.0f, 1.1f, 1.6f},
        {7.0f, 10.0f, 22.0f}, {4.0f, 2.0f, 1.0f}, {6.0f, 3.1f, 22.6f}
    };

    const SphereBoxBounds FromPoints(ArrayPoints);
    const SphereBoxBounds FromSphereBox(Vector3::UNIT, Vector3::UNIT * 5, 6.f);
    const SphereBoxBounds FromSphereBoxTRUE(Vector3::UNIT * -46.f, Vector3::UNIT * 5, 6.f);
    REQUIRE(FromPoints.center == Vector3(-46.5000000f, 4.5f, 10.30000002f));
    REQUIRE(FromPoints.boxExtent == Vector3(53.5f, 5.5f, 12.30000002f));

    REQUIRE_FALSE(Math::spheresIntersect(FromPoints, FromSphereBox));
    REQUIRE_FALSE(Math::boxesIntersect(FromPoints, FromSphereBoxTRUE));
}

TEST_CASE("chUtilities - Utilities") {
    class Submodule : public Module<Submodule> {
    public:
        int TestNumber = 11552;
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
    // WeakPtr<DynamicLibrary> TestDll = DynamicLibraryManager::instance().loadDynLibrary("DllTest");
    // SPtr<DynamicLibrary> RealPtr = TestDll.lock();
    // REQUIRE(RealPtr);

    // using SymbolFromDll = void(*)();
    // SymbolFromDll func = reinterpret_cast<SymbolFromDll>(RealPtr->getSymbol("testFunction"));

    // REQUIRE(func);
    // func();

    CH_LOG_DEBUG("Test Message");

    Event<int(int, float)> Onsomething;
    HEvent listener1 = Onsomething.connect([](int a, float b)->int {
        REQUIRE(a == 10);
        REQUIRE(b == 125.55f);
        return 1;
    });

    struct Test {
        Test(int a, float b) : A(a), B(b) {}
        int A;
        float B;

        int foo(int a, float b) {
            REQUIRE(a == 10);
            REQUIRE(b == 125.55f);
            return 0;
        }
    };

    auto* TestClass = new Test(123, 35445.64565f);

    HEvent listener2 = Onsomething.connect([&TestClass](int a, float b)->int {
        REQUIRE(TestClass->A == 123);
        REQUIRE(TestClass->B == 35445.64565f);
        TestClass->foo(a, b);
        return 1;
    });

    HEvent listener3 = Onsomething.connect(std::bind(&Test::foo, TestClass,
        std::placeholders::_1,
        std::placeholders::_2));

    Onsomething(10, 125.55f);
}

TEST_CASE("chUtilities - StringAndUTF8") {
    const WString TestWString(UTF8::toUTF16("Created as wide string"));
    const String WellPerformedConvertion("Created as wide string");

    REQUIRE(UTF8::fromWide(TestWString) == WellPerformedConvertion);
    REQUIRE(UTF8::toWide(WellPerformedConvertion) == TestWString);

    const String ReplaceTestString("Test-string-that-should-replace-all-hyphens");
    const String WellPerformedReplacing("Test string that should replace all hyphens");
    REQUIRE(StringUtils::replaceAllChars(ReplaceTestString, '-', ' ') == WellPerformedReplacing);

    const String ReplaceSubStringtest("Test that asdfg contains asdfg some asdfg substrings to erase");
    const String WellPerformedSubStrReplace("Test that sd contains sd some sd substrings to erase");
    REQUIRE(StringUtils::replaceAllSubStr(ReplaceSubStringtest, "asdfg", "sd") == WellPerformedSubStrReplace);

    const String ToSplitChar("This-is-a-test-that-should-split-by-hyphens");
    REQUIRE(StringUtils::splitString(ToSplitChar, '-').size() == 9);

    const String ToSplitString("This123is123a123test123that123should123split123by123hyphen");
    REQUIRE(StringUtils::splitString(ToSplitString, "123").size() == 9);

    String Formated = StringUtils::format("hello world, {0}, {0}, {1}, {2}, {3}",
        123, 1.23, "123", String("123"));

    const String Anwser("hello world, 123, 123, 1.230000, 123, 123");
    REQUIRE(Anwser == Formated);

    // Platform-specific paths
#if USING(CH_PLATFORM_WIN32)
    const Path TestPath("C:\\Users\\Public\\");
    Path WrongTest("C:/Users/Public/NoExist/");
    const Path ToCreate("C:/Users/Public/Test/");
    const Path ToCreateMany("C:/Users/Public/Test/a/b/c/");
    REQUIRE(TestPath.toString() == "C:/Users/Public/");
    REQUIRE(WrongTest.toString() == "C:/Users/Public/NoExist/");
#elif USING(CH_PLATFORM_LINUX)
    const Path TestPath("/home/accelmr/Public/");
    Path WrongTest("/home/accelmr/Public/NoExist/");
    const Path ToCreate("/home/accelmr/Public/Test/");
    const Path ToCreateMany("/home/accelmr/Public/Test/a/b/c/");

    REQUIRE(TestPath.toString() == "/home/accelmr/Public/");
    REQUIRE(WrongTest.toString() == "/home/accelmr/Public/NoExist/");
#endif

    Path Relativepath2("./Here/path");
    Path Relativepath3("Here/path");

    REQUIRE(Relativepath2.toString() == "./Here/path");
    REQUIRE(Relativepath3.toString() == "Here/path");

    REQUIRE(FileSystem::createDirectory(ToCreate));

    REQUIRE(FileSystem::createDirectories(ToCreateMany));

    SPtr<DataStream> memStream = chMakeShared<MemoryDataStream>(8);
    REQUIRE(memStream->size() == 8);

    int Test1110 = 110;
    memStream->write(&Test1110, sizeof(int));
    *memStream << 18;

    SIZE_T moved = memStream->tell();
    REQUIRE(moved == 8);
    memStream->seek(0);

    int InTest110 = 0;
    memStream->read(&InTest110, sizeof(int));
    REQUIRE(InTest110 == 110);

    int Intest18;
    *memStream >> Intest18;
    REQUIRE(Intest18 == 18);

    REQUIRE_FALSE(memStream->isFile());
    REQUIRE(memStream->isReadable());
    REQUIRE(memStream->isWriteable());

    Path testFilePath(ToCreateMany.toString() + "testFile.test");
    FileSystem::dumpMemStreamIntoFile(memStream, testFilePath);

    auto readFileSteam = FileSystem::openFile(testFilePath);
    REQUIRE(readFileSteam);

    int FileInTest110 = 0;
    readFileSteam->read(&FileInTest110, sizeof(int));
    REQUIRE(FileInTest110 == 110);

    int FileIntest18;
    *readFileSteam >> FileIntest18;
    REQUIRE(FileIntest18 == 18);

    memStream.reset();
    REQUIRE_FALSE(memStream);

    readFileSteam->close();

    const Path newFilePath(ToCreateMany.toString() + "testFile2.test2");
    SPtr<DataStream> newFile = FileSystem::createAndOpenFile(newFilePath);

    *newFile << "This is a test to see if write() is ok" << " concat ok " << "\n";
    *newFile << "This must be a new line";
    newFile->close();

    auto readFile = FileSystem::openFile(newFilePath);
    String fromFile = readFile->getAsString();

    const String mustBe("This is a test to see if write() is ok\xEF\xBB\xBF concat ok \xEF\xBB\xBF\n\xEF\xBB\xBFThis must be a new line");
    REQUIRE(mustBe == fromFile);

    readFile->close();

    REQUIRE_THROWS_AS(FileSystem::remove(ToCreate), std::exception);
    REQUIRE(FileSystem::removeAll(ToCreate));

    /*
    const Path DebugFile("C:/Users/Public/debug.txt");
    CH_LOG_DEBUG("TEST");
    CH_LOG_ERROR("TEST");
    CH_LOG_WARNING("TEST");
    g_Debug().saveLog(DebugFile);*/
}


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

TEST_CASE("CommandParser Tests", "[CommandParser]") {

    SECTION("TestParse") {
        // Setup
        const char* argv[] = { "program_name", "-option1=value1", "-option2=value2", "-flag" };
        int32 argc = 4;

        CommandParser& parser = CommandParser::getInstance();
        parser.parse(argc, (char**)argv);

        // Test parsed parameters
        REQUIRE(parser.getParam("option1") == "value1");
        REQUIRE(parser.getParam("option2") == "value2");

        // Test flag
        REQUIRE(parser.isFlagSet("flag") == true);
    }

    SECTION("TestDefaultParam") {
        // Setup
        const char* argv[] = { "program_name" };
        int32 argc = 1;

        CommandParser& parser = CommandParser::getInstance();
        parser.parse(argc, (char**)argv);

        // Test default parameter
        REQUIRE(parser.getParam("option_not_present", "default_value") == "default_value");
    }

    SECTION("TestMissingFlag") {
        // Setup
        const char* argv[] = { "program_name" };
        int32 argc = 1;

        CommandParser& parser = CommandParser::getInstance();
        parser.parse(argc, (char**)argv);

        // Test missing flag
        REQUIRE(parser.isFlagSet("flag_not_present") == false);
    }

    SECTION("TestCaseInsensitive") {
        // Setup
        const char* argv[] = { "program_name", "-OPTION=value" };
        int32 argc = 2;

        CommandParser& parser = CommandParser::getInstance();
        parser.parse(argc, (char**)argv);

        // Test case insensitivity
        REQUIRE(parser.getParam("option") == "value");
    }
}

#endif //CH_PLATFORM_WINDOWS

int _main(int argc, char** argv) {
    return 0;
}
