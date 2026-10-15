/************************************************************************/
/**
 * @file chLightComponent.cpp
 * @author AccelMR
 * @date 2026/10/13
 * @brief
 *  Components that light the scene from their GameObject.
 */
/************************************************************************/
#include "chLightComponent.h"

#include "chFileStream.h"
#include "chGameObject.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chScene.h"

namespace chEngineSDK {

namespace {
// A cone of 90 degrees or more is a half space, which the falloff between the angles
// cannot describe.
constexpr float kMaxConeAngle = 89.0f;

struct LightFileData
{
  float color[3];
  float intensity;
};
static_assert(sizeof(LightFileData) == 16, "LightFileData must have no padding");

struct SpotLightFileData
{
  float range;
  // Half angles, in degrees.
  float innerConeAngle;
  float outerConeAngle;
};
static_assert(sizeof(SpotLightFileData) == 12, "SpotLightFileData must have no padding");
} // namespace

/*
 */
LightComponent::LightComponent(LightType type, float intensity, bool bCastShadows)
  : m_type(type),
    m_intensity(intensity),
    m_bCastShadows(bCastShadows)
{}

/*
 */
void
LightComponent::setColor(const LinearColor& color)
{
  m_color = color;
  updateLightItem();
}

/*
 */
void
LightComponent::setIntensity(float intensity)
{
  m_intensity = Math::max(intensity, 0.0f);
  updateLightItem();
}

/*
 */
void
LightComponent::setCastShadows(bool bCastShadows)
{
  m_bCastShadows = bCastShadows;
  updateLightItem();
}

/*
 */
void
LightComponent::serialize(DataStream& stream) const
{
  const uint32 version = SERIALIZATION_VERSION;
  const LightFileData data{.color = {m_color.r, m_color.g, m_color.b},
                           .intensity = m_intensity};
  const uint8 castShadows = m_bCastShadows ? 1 : 0;
  stream.write(&version, sizeof(version));
  stream.write(&data, sizeof(data));
  stream.write(&castShadows, sizeof(castShadows));
}

/*
 */
bool
LightComponent::deserialize(DataStream& stream)
{
  uint32 version = 0;
  LightFileData data;
  if (stream.read(&version, sizeof(version)) != sizeof(version) || version == 0 ||
      version > SERIALIZATION_VERSION || stream.read(&data, sizeof(data)) != sizeof(data)) {
    return false;
  }
  // Version 1 had no shadow flag, so those lights keep the default of their type.
  if (version >= 2) {
    uint8 castShadows = 0;
    if (stream.read(&castShadows, sizeof(castShadows)) != sizeof(castShadows)) {
      return false;
    }
    setCastShadows(castShadows != 0);
  }
  setColor(LinearColor(data.color[0], data.color[1], data.color[2], 1.0f));
  setIntensity(data.intensity);
  return true;
}

/*
 */
void
LightComponent::onRegister()
{
  m_lightId =
      getOwner()->getScene()->addLight(LightItem{.type = m_type, .owner = getOwner()});
  updateLightItem();
}

/*
 */
void
LightComponent::onUnregister()
{
  getOwner()->getScene()->removeLight(m_lightId);
}

/*
 */
void
LightComponent::onTransformChanged()
{
  updateLightItem();
}

/*
 */
void
LightComponent::writeTypeValues(LightItem& item) const
{
  CH_PARAMETER_UNUSED(item);
}

/*
 */
void
LightComponent::updateLightItem()
{
  if (!isRegistered()) {
    return;
  }

  LightItem& item = getOwner()->getScene()->getLight(m_lightId);
  // Row vectors: row 0 is where the local forward axis ends up, row 3 the position.
  const Matrix4& world = getOwner()->getTransform().getWorldMatrix();
  Vector3 direction(world[0][0], world[0][1], world[0][2]);
  if (!direction.normalize()) {
    direction = Vector3::FORWARD;
  }
  item.position = Vector3(world[3][0], world[3][1], world[3][2]);
  item.direction = direction;
  item.color = LinearColor(m_color.r * m_intensity, m_color.g * m_intensity,
                           m_color.b * m_intensity, 1.0f);
  item.bCastShadows = m_bCastShadows;
  writeTypeValues(item);
}

/*
 */
DirectionalLightComponent::DirectionalLightComponent()
  : LightComponent(LightType::Directional, 3.0f, true)
{}

/*
 */
PointLightComponent::PointLightComponent()
  : LightComponent(LightType::Point, 10.0f, false)
{}

/*
 */
void
PointLightComponent::setRange(float range)
{
  m_range = Math::max(range, 0.0f);
  updateLightItem();
}

/*
 */
void
PointLightComponent::serialize(DataStream& stream) const
{
  LightComponent::serialize(stream);
  stream.write(&m_range, sizeof(m_range));
}

/*
 */
bool
PointLightComponent::deserialize(DataStream& stream)
{
  float range = 0.0f;
  if (!LightComponent::deserialize(stream) ||
      stream.read(&range, sizeof(range)) != sizeof(range)) {
    return false;
  }
  setRange(range);
  return true;
}

/*
 */
void
PointLightComponent::writeTypeValues(LightItem& item) const
{
  item.range = m_range;
}

/*
 */
SpotLightComponent::SpotLightComponent()
  : LightComponent(LightType::Spot, 10.0f, false)
{}

/*
 */
void
SpotLightComponent::setRange(float range)
{
  m_range = Math::max(range, 0.0f);
  updateLightItem();
}

/*
 */
void
SpotLightComponent::setConeAngles(const Degree& inner, const Degree& outer)
{
  const float outerDegrees = Math::clamp(outer.valueDegree(), 0.0f, kMaxConeAngle);
  m_outerConeAngle = Degree(outerDegrees);
  m_innerConeAngle = Degree(Math::clamp(inner.valueDegree(), 0.0f, outerDegrees));
  updateLightItem();
}

/*
 */
void
SpotLightComponent::serialize(DataStream& stream) const
{
  LightComponent::serialize(stream);
  const SpotLightFileData data{.range = m_range,
                               .innerConeAngle = m_innerConeAngle.valueDegree(),
                               .outerConeAngle = m_outerConeAngle.valueDegree()};
  stream.write(&data, sizeof(data));
}

/*
 */
bool
SpotLightComponent::deserialize(DataStream& stream)
{
  SpotLightFileData data;
  if (!LightComponent::deserialize(stream) ||
      stream.read(&data, sizeof(data)) != sizeof(data)) {
    return false;
  }
  setRange(data.range);
  setConeAngles(Degree(data.innerConeAngle), Degree(data.outerConeAngle));
  return true;
}

/*
 */
void
SpotLightComponent::writeTypeValues(LightItem& item) const
{
  item.range = m_range;
  item.innerConeCos = Math::cos(m_innerConeAngle);
  item.outerConeCos = Math::cos(m_outerConeAngle);
}

} // namespace chEngineSDK
