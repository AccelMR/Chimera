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

#include "chGameObject.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chScene.h"

namespace chEngineSDK {

namespace {
// A cone of 90 degrees or more is a half space, which the falloff between the angles
// cannot describe.
constexpr float kMaxConeAngle = 89.0f;
} // namespace

/*
 */
LightComponent::LightComponent(LightType type, float intensity)
  : m_type(type),
    m_intensity(intensity)
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
LightComponent::onRegister()
{
  m_lightId = getOwner()->getScene()->addLight(LightItem{.type = m_type});
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
  writeTypeValues(item);
}

/*
 */
DirectionalLightComponent::DirectionalLightComponent()
  : LightComponent(LightType::Directional, 3.0f)
{}

/*
 */
PointLightComponent::PointLightComponent()
  : LightComponent(LightType::Point, 10.0f)
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
PointLightComponent::writeTypeValues(LightItem& item) const
{
  item.range = m_range;
}

/*
 */
SpotLightComponent::SpotLightComponent()
  : LightComponent(LightType::Spot, 10.0f)
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
SpotLightComponent::writeTypeValues(LightItem& item) const
{
  item.range = m_range;
  item.innerConeCos = Math::cos(m_innerConeAngle);
  item.outerConeCos = Math::cos(m_outerConeAngle);
}

} // namespace chEngineSDK
