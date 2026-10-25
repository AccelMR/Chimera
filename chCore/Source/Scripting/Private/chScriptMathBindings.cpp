/************************************************************************/
/**
 * @file chScriptMathBindings.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  The math types every script sees.
 */
/************************************************************************/
#include "chScriptBindings.h"

#include "chMath.h"
#include "chScriptMathTypes.h"

namespace chEngineSDK {
namespace {
/*
 * Methods that change the value they are called on and return it, so scripts can chain them
 * and reuse one vector every frame instead of making a new one per operation.
 */
template<typename V>
V&
copyFrom(V& self, const V& other)
{
  self = other;
  return self;
}

template<typename V>
V&
addInPlace(V& self, const V& other)
{
  self += other;
  return self;
}

template<typename V>
V&
subtractInPlace(V& self, const V& other)
{
  self -= other;
  return self;
}

template<typename V>
V&
scaleInPlace(V& self, const float scalar)
{
  self *= scalar;
  return self;
}

template<typename V>
V&
addScaledInPlace(V& self, const V& other, const float scalar)
{
  self += other * scalar;
  return self;
}

template<typename V>
V&
addVectors(V& self, const V& a, const V& b)
{
  self = a + b;
  return self;
}

template<typename V>
V&
subtractVectors(V& self, const V& a, const V& b)
{
  self = a - b;
  return self;
}

template<typename V>
V&
normalizeInPlace(V& self)
{
  self.normalize();
  return self;
}

template<typename V>
V&
clampLengthInPlace(V& self, const float maxLength)
{
  const float squaredLength = self.sqrMagnitude();
  if (squaredLength > maxLength * maxLength && squaredLength > 0.0f) {
    self *= maxLength / Math::sqrt(squaredLength);
  }
  return self;
}

template<typename V>
V&
lerpInPlace(V& self, const V& target, const float alpha)
{
  self = self + (target - self) * alpha;
  return self;
}

/*
 * Methods that give a new value.
 */
template<typename V>
V
copyOf(const V& self)
{
  return self;
}

template<typename V>
V
normalizedOf(const V& self)
{
  return self.getNormalized();
}

template<typename V>
bool
nearEqualTo(const V& self, const V& other)
{
  return self.nearEqual(other);
}

template<typename V>
float
lengthOf(const V& self)
{
  return self.magnitude();
}

template<typename V>
float
squaredLengthOf(const V& self)
{
  return self.sqrMagnitude();
}

template<typename V>
V
lerpOf(const V& a, const V& b, const float alpha)
{
  return a + (b - a) * alpha;
}

Vector2&
setVector2(Vector2& self, const float x, const float y)
{
  self.x = x;
  self.y = y;
  return self;
}

String
vector2ToString(const Vector2& self)
{
  return self.toString();
}

Vector3&
setVector3(Vector3& self, const float x, const float y, const float z)
{
  self.x = x;
  self.y = y;
  self.z = z;
  return self;
}

Vector3&
crossVectors(Vector3& self, const Vector3& a, const Vector3& b)
{
  self = a.cross(b);
  return self;
}

Vector3&
rotateVectorInPlace(Vector3& self, const Quaternion& rotation)
{
  self = rotation.rotateVector(self);
  return self;
}

float
distanceBetween(const Vector3& a, const Vector3& b)
{
  return a.distance(b);
}

float
squaredDistanceBetween(const Vector3& a, const Vector3& b)
{
  return a.sqrDistance(b);
}

String
vector3ToString(const Vector3& self)
{
  return self.toString();
}

Vector4&
setVector4(Vector4& self, const float x, const float y, const float z, const float w)
{
  self = Vector4(x, y, z, w);
  return self;
}

String
vector4ToString(const Vector4& self)
{
  return self.toString();
}

Quaternion
makeIdentity()
{
  return Quaternion::IDENTITY;
}

Quaternion
makeQuaternion(const float x, const float y, const float z, const float w)
{
  return Quaternion(x, y, z, w);
}

Quaternion
quaternionFromEuler(const float pitch, const float yaw, const float roll)
{
  return Quaternion(Rotator(pitch, yaw, roll));
}

Quaternion
quaternionFromAxisAngle(const Vector3& axis, const float degrees)
{
  return Quaternion(axis, Degree(degrees));
}

// Functions exported from chUtilities are wrapped: the address of a function imported from a
// DLL is not a constant every compiler accepts as a template argument.
Rotator
toEuler(const Quaternion& self)
{
  return self.toRotator();
}

Quaternion
toQuaternion(const Rotator& self)
{
  return self.toQuaternion();
}

Quaternion
inverseOf(const Quaternion& self)
{
  return self.getInverse();
}

Quaternion&
setQuaternion(Quaternion& self, const float x, const float y, const float z, const float w)
{
  self = Quaternion(x, y, z, w);
  return self;
}

Quaternion&
multiplyInPlace(Quaternion& self, const Quaternion& other)
{
  self *= other;
  return self;
}

Quaternion
slerpOf(const Quaternion& a, const Quaternion& b, const float alpha)
{
  return Quaternion::slerp(a, b, alpha);
}

Quaternion
nlerpOf(const Quaternion& a, const Quaternion& b, const float alpha)
{
  return Quaternion::nlerp(a, b, alpha);
}

String
quaternionToString(const Quaternion& self)
{
  return self.toString();
}

Rotator
normalizedRotator(const Rotator& self)
{
  return self.getNormalized();
}

String
rotatorToString(const Rotator& self)
{
  return self.toString();
}

LinearColor&
setColor(LinearColor& self, const float r, const float g, const float b, const float a)
{
  self = LinearColor(r, g, b, a);
  return self;
}

LinearColor
lerpColor(const LinearColor& from, const LinearColor& to, const float alpha)
{
  return LinearColor::lerp(from, to, alpha);
}

String
colorToString(const LinearColor& self)
{
  return self.toString();
}

/*
 * What every vector type has, named the same in each.
 */
template<typename V>
void
addVectorMembers(ScriptClass<V>& vectorClass)
{
  vectorClass.template method<&V::dot>("dot", "other")
      .template method<&lengthOf<V>>("length")
      .template method<&squaredLengthOf<V>>("lengthSquared")
      .template method<&normalizedOf<V>>("normalized")
      .template method<&nearEqualTo<V>>("nearEqual", "other")
      .template method<&copyOf<V>>("copy")
      .template method<&copyFrom<V>>("copyFrom", "other")
      .template method<&addInPlace<V>>("add", "other")
      .template method<&subtractInPlace<V>>("sub", "other")
      .template method<&scaleInPlace<V>>("scale", "scalar")
      .template method<&addScaledInPlace<V>>("addScaled", "other", "scalar")
      .template method<&addVectors<V>>("addVectors", "a", "b")
      .template method<&subtractVectors<V>>("subVectors", "a", "b")
      .template method<&normalizeInPlace<V>>("normalize")
      .template method<&clampLengthInPlace<V>>("clampLength", "maxLength")
      .template method<&lerpInPlace<V>>("lerp", "target", "alpha")
      .template staticFunction<&lerpOf<V>>("Lerp", "a", "b", "alpha")
      .template operators<ScriptOperator::Add, ScriptOperator::Subtract,
                          ScriptOperator::Multiply, ScriptOperator::Negate,
                          ScriptOperator::Equal>();
}
} // namespace

/*
 */
void
ScriptBindings::registerMath(lua_State* state)
{
  {
    ScriptClass<Vector2> vector2(state);
    vector2.constructor<>()
        .constructor<float, float>("x", "y")
        .field<&Vector2::x>("x")
        .field<&Vector2::y>("y")
        .method<&Vector2::cross>("cross", "other")
        .method<&setVector2>("set", "x", "y")
        .metamethod<ScriptOperator::ToString, &vector2ToString>()
        .constant("ZERO", Vector2::ZERO)
        .constant("ONE", Vector2::UNIT)
        .constant("UNIT_X", Vector2::UNIT_X)
        .constant("UNIT_Y", Vector2::UNIT_Y);
    addVectorMembers(vector2);
  }
  {
    ScriptClass<Vector3> vector3(state);
    vector3.constructor<>()
        .constructor<float, float, float>("x", "y", "z")
        .field<&Vector3::x>("x")
        .field<&Vector3::y>("y")
        .field<&Vector3::z>("z")
        .method<&Vector3::cross>("cross", "other")
        .method<&distanceBetween>("distance", "other")
        .method<&squaredDistanceBetween>("distanceSquared", "other")
        .method<&setVector3>("set", "x", "y", "z")
        .method<&crossVectors>("crossVectors", "a", "b")
        .method<&rotateVectorInPlace>("rotate", "rotation")
        .operators<ScriptOperator::Divide>()
        .metamethod<ScriptOperator::ToString, &vector3ToString>()
        .constant("ZERO", Vector3::ZERO)
        .constant("ONE", Vector3::UNIT)
        .constant("FORWARD", Vector3::FORWARD)
        .constant("BACKWARD", Vector3::BACKWARD)
        .constant("RIGHT", Vector3::RIGHT)
        .constant("LEFT", Vector3::LEFT)
        .constant("UP", Vector3::UP)
        .constant("DOWN", Vector3::DOWN);
    addVectorMembers(vector3);
  }
  {
    ScriptClass<Vector4> vector4(state);
    vector4.constructor<>()
        .constructor<float, float, float, float>("x", "y", "z", "w")
        .field<&Vector4::x>("x")
        .field<&Vector4::y>("y")
        .field<&Vector4::z>("z")
        .field<&Vector4::w>("w")
        .method<&setVector4>("set", "x", "y", "z", "w")
        .metamethod<ScriptOperator::ToString, &vector4ToString>()
        .constant("ZERO", Vector4::ZERO)
        .constant("ONE", Vector4::UNIT);
    addVectorMembers(vector4);
  }

  ScriptClass<Quaternion>(state)
      .constructorFunction<&makeIdentity>()
      .constructorFunction<&makeQuaternion>("x", "y", "z", "w")
      .field<&Quaternion::x>("x")
      .field<&Quaternion::y>("y")
      .field<&Quaternion::z>("z")
      .field<&Quaternion::w>("w")
      .method<&Quaternion::dot>("dot", "other")
      .method<&toEuler>("toEuler")
      .method<&Quaternion::rotateVector>("rotateVector", "vector")
      .method<&Quaternion::unrotateVector>("unrotateVector", "vector")
      .method<&inverseOf>("inverse")
      .method<&normalizedOf<Quaternion>>("normalized")
      .method<&nearEqualTo<Quaternion>>("nearEqual", "other")
      .method<&copyOf<Quaternion>>("copy")
      .method<&copyFrom<Quaternion>>("copyFrom", "other")
      .method<&setQuaternion>("set", "x", "y", "z", "w")
      .method<&normalizeInPlace<Quaternion>>("normalize")
      .method<&multiplyInPlace>("multiply", "other")
      .staticFunction<&quaternionFromEuler>("FromEuler", "pitch", "yaw", "roll")
      .staticFunction<&quaternionFromAxisAngle>("FromAxisAngle", "axis", "degrees")
      .staticFunction<&slerpOf>("Slerp", "a", "b", "alpha")
      .staticFunction<&nlerpOf>("Nlerp", "a", "b", "alpha")
      .operators<ScriptOperator::Multiply, ScriptOperator::Equal>()
      .metamethod<ScriptOperator::ToString, &quaternionToString>()
      .constant("IDENTITY", Quaternion::IDENTITY);

  ScriptClass<Rotator>(state)
      .constructor<>()
      .constructor<float, float, float>("pitch", "yaw", "roll")
      .field<&Rotator::pitch>("pitch")
      .field<&Rotator::yaw>("yaw")
      .field<&Rotator::roll>("roll")
      .method<&toQuaternion>("toQuaternion")
      .method<&normalizedRotator>("normalized")
      .method<&copyOf<Rotator>>("copy")
      .operators<ScriptOperator::Add, ScriptOperator::Subtract, ScriptOperator::Multiply,
                 ScriptOperator::Equal>()
      .metamethod<ScriptOperator::ToString, &rotatorToString>()
      .constant("ZERO", Rotator::ZERO);

  ScriptClass<LinearColor>(state)
      .constructor<>()
      .constructor<float, float, float>("r", "g", "b")
      .constructor<float, float, float, float>("r", "g", "b", "a")
      .field<&LinearColor::r>("r")
      .field<&LinearColor::g>("g")
      .field<&LinearColor::b>("b")
      .field<&LinearColor::a>("a")
      .method<&setColor>("set", "r", "g", "b", "a")
      .method<&copyOf<LinearColor>>("copy")
      .staticFunction<&lerpColor>("Lerp", "from", "to", "alpha")
      .operators<ScriptOperator::Add, ScriptOperator::Subtract, ScriptOperator::Multiply,
                 ScriptOperator::Equal>()
      .metamethod<ScriptOperator::ToString, &colorToString>()
      .constant("WHITE", LinearColor::WHITE)
      .constant("BLACK", LinearColor::BLACK)
      .constant("GRAY", LinearColor::GRAY)
      .constant("RED", LinearColor::RED)
      .constant("GREEN", LinearColor::GREEN)
      .constant("BLUE", LinearColor::BLUE)
      .constant("YELLOW", LinearColor::YELLOW)
      .constant("CYAN", LinearColor::CYAN)
      .constant("MAGENTA", LinearColor::MAGENTA)
      .constant("TRANSPARENT", LinearColor::TRANSPARENT_BLACK);
}

} // namespace chEngineSDK
