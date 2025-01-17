/************************************************************************/
/**
 * @file chQuaternion.cpp
 * @author AccelMR
 * @date 2022/03/29
 *   Quaternion file hold implementation of Quaternion externals.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chQuaternion.h"

#include "chMath.h"
#include "chMath.h"
#include "chRotator.h"
#include "chMatrix4.h"
#include "chRadian.h"

namespace chEngineSDK {

const Quaternion Quaternion::IDENTITY = Quaternion(0.f, 0.f, 0.f, 1.f);

/*
*/
Quaternion::Quaternion(const Vector3 &axis, const Degree& angle)
{
  const float half_a = 0.5f * angle.valueRadian();
  float s, c;
  Math::sin_cos(&s, &c, half_a);

  x = s * axis.x;
  y = s * axis.y;
  z = s * axis.z;
  w = c;

  diagnosticCheckNaN();
}

/*
*/
Quaternion::Quaternion(const Vector4& v4) 
  : x(v4.x), y(v4.y), z(v4.z), w(v4.w)
{
    diagnosticCheckNaN();
}

/*
*/
Rotator
Quaternion::toRotator() const
{
  diagnosticCheckNaN();
  //Reference:
  //http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  //https://github.com/YclepticStudios/gmath/blob/master/src/Quaternion.hpp

  //Most important
  //http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
  //TODO: check for non-normalized quaternions in the link above

  const float SINGULARITY_THRESHOLD = 0.4999995f;
  float SingularityTest = x * y + z * w;
  Rotator R;

  if (SingularityTest > SINGULARITY_THRESHOLD) { // singularity at north pole
    R.yaw = 90.0f; // Pi / 2
    R.pitch = 2.0f * Math::atan2(x, w).valueDegree();
    R.roll = 0;
  }
  else if (SingularityTest < -SINGULARITY_THRESHOLD) { // singularity at south pole
    R.yaw = -90.0f; // Pi / 2
    R.pitch = -2.0f * Math::atan2(x, w).valueDegree();
    R.roll = 0;
  }
  else
  {
    const float sqx = Math::pow(x, 2.0f);
    const float sqy = Math::pow(y, 2.0f);
    const float sqz = Math::pow(z, 2.0f);
    
    R.yaw =  Math::asin(2.0f * SingularityTest);
    R.pitch =  Math::atan2(2.0f * y * w - 2.0f * x * z, 1 - 2.0f * sqy - 2.0f * sqz);
    R.roll =  Math::atan2(2.0f * x * w - 2.0f * y * z, 1 - 2.0f * sqx - 2.0f * sqz);
  }


# if USING(CH_DEBUG_MODE)
  if (R.checkIfNaN()) {
    //TODO FIx this
    //LOG_WARN("Quaternion::rotator(): Rotator result contains NaN!");
    R = Rotator::ZERO;
}
# endif

   return R;
}

/*
*/
Vector3
Quaternion::rotateVector(const Vector3& v) const
{
  //http://people.csail.mit.edu/bkph/articles/Quaternions.pdf
  //V' = V + 2w(Q x V) + (2Q x (Q x V))
  //refactor:
  //V' = V + w(2(Q x V)) + (Q x (2(Q x V)))
  //T = 2(Q x V);
  //V' = V + w*(T) + (Q x T)
  const Vector3 Q(x, y, z);
  const Vector3 T = 2.f * Q.cross(v);
  const Vector3 Result = v + (w * T) + Q.cross(T);
  return Result;
}

/*
*/
Vector3
Quaternion::unrotateVector(const Vector3& v) const
{
  const Vector3 Q(-x, -y, -z);  //Inverse
  const Vector3 T = 2.f * Q.cross(v);
  const Vector3 Result = v + (w * T) + Q.cross(T);
  return Result;
}

/*
*/
Quaternion::Quaternion(const Rotator& rotator)
{
  *this = rotator.toQuaternion();
  diagnosticCheckNaN();
}

/*
*/
Quaternion::Quaternion(const Matrix4& m4)
{
  float	s;

  //Check diagonal (trace)
  const float tr = m4.m00 + m4.m11 + m4.m22;

  if (0.0f < tr) {
    const float InvS = Math::invSqrt(tr + 1.f);
    w = 0.5f * (1.f / InvS);
    s = 0.5f * InvS;

    x = (m4.m12 - m4.m21) * s;
    y = (m4.m20 - m4.m02) * s;
    z = (m4.m01 - m4.m10) * s;
  }
  else {
    //diagonal is negative
    int32 i = 0;

    if (m4.m11 > m4.m00) {
      i = 1;
    }

    if (m4.m22 > m4.m4x4[i][i]) {
      i = 2;
    }

    static const int32 nxt[3] = {1, 2, 0};
    const int32 j = nxt[i];
    const int32 k = nxt[j];

    s = m4.m4x4[i][i] - m4.m4x4[j][j] - m4.m4x4[k][k] + 1.0f;

    const float InvS = Math::invSqrt(s);

    float qt[4];
    qt[i] = 0.5f * (1.f / InvS);

    s = 0.5f * InvS;

    qt[3] = (m4.m4x4[j][k] - m4.m4x4[k][j]) * s;
    qt[j] = (m4.m4x4[i][j] + m4.m4x4[j][i]) * s;
    qt[k] = (m4.m4x4[i][k] + m4.m4x4[k][i]) * s;

    x = qt[0];
    y = qt[1];
    z = qt[2];
    w = qt[3];

  }
  diagnosticCheckNaN();
}
}


