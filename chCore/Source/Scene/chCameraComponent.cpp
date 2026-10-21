/************************************************************************/
/**
 * @file chCameraComponent.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Component that a game views the scene through.
 */
/************************************************************************/
#include "chCameraComponent.h"

#include "chFileStream.h"
#include "chGameObject.h"
#include "chMath.h"
#include "chScene.h"

namespace chEngineSDK {

namespace {
struct CameraFileData
{
  uint32 projectionType;
  float fieldOfViewDegrees;
  float nearPlane;
  float farPlane;
  float orthographicSize;
};
static_assert(sizeof(CameraFileData) == 20, "CameraFileData must have no padding");

constexpr float kMinFieldOfView = 1.0f;
constexpr float kMaxFieldOfView = 170.0f;
constexpr float kMinNearPlane = 0.001f;
constexpr float kMinOrthographicSize = 0.001f;
// Camera builds its view with world Z up, which breaks when looking straight up or down.
constexpr float kMaxPitch = 89.0f;
} // namespace

/*
 */
void
CameraComponent::setProjectionType(CameraProjectionType type)
{
  m_projectionType = type;
}

/*
 */
void
CameraComponent::setFieldOfView(Degree fieldOfView)
{
  m_fieldOfView =
      Degree(Math::clamp(fieldOfView.valueDegree(), kMinFieldOfView, kMaxFieldOfView));
}

/*
 */
void
CameraComponent::setClipPlanes(float nearPlane, float farPlane)
{
  m_nearPlane = Math::max(nearPlane, kMinNearPlane);
  m_farPlane = Math::max(farPlane, m_nearPlane * 2.0f);
}

/*
 */
void
CameraComponent::setOrthographicSize(float halfHeight)
{
  m_orthographicSize = Math::max(halfHeight, kMinOrthographicSize);
}

/*
 */
void
CameraComponent::fillCamera(Camera& outCamera, float viewportWidth,
                            float viewportHeight) const
{
  // Row vectors: row 0 of the world matrix is where the local forward axis ends up, row 3
  // the position.
  const Matrix4& world = getOwner()->getTransform().getWorldMatrix();
  const Vector3 position(world[3][0], world[3][1], world[3][2]);
  Vector3 forward(world[0][0], world[0][1], world[0][2]);
  if (!forward.normalize()) {
    forward = Vector3::FORWARD;
  }

  // Looking almost straight up or down leaves no yaw in the forward axis, so it is read
  // from the right axis, which stays level.
  float yawDegrees = Math::atan2(forward.y, forward.x).valueDegree();
  if (forward.x * forward.x + forward.y * forward.y < Math::SMALL_NUMBER) {
    yawDegrees = Math::atan2(world[1][1], world[1][0]).valueDegree() - 90.0f;
  }
  const float pitchDegrees =
      Math::clamp(Math::asin(forward.z).valueDegree(), -kMaxPitch, kMaxPitch);

  float sinPitch, cosPitch, sinYaw, cosYaw;
  Math::sinCos(Degree(pitchDegrees).valueRadian(), sinPitch, cosPitch);
  Math::sinCos(Degree(yawDegrees).valueRadian(), sinYaw, cosYaw);
  const Vector3 viewDirection(cosPitch * cosYaw, cosPitch * sinYaw, sinPitch);

  outCamera.setPosition(position);
  outCamera.lookAt(position + viewDirection);
  outCamera.setProjectionType(m_projectionType);
  outCamera.setFieldOfView(Radian(m_fieldOfView));
  outCamera.setClipPlanes(m_nearPlane, m_farPlane);
  outCamera.setOrthographicSize(m_orthographicSize);
  outCamera.setViewportSize(viewportWidth, viewportHeight);
}

/*
 */
void
CameraComponent::serialize(DataStream& stream) const
{
  const uint32 version = SERIALIZATION_VERSION;
  const CameraFileData data{.projectionType = static_cast<uint32>(m_projectionType),
                            .fieldOfViewDegrees = m_fieldOfView.valueDegree(),
                            .nearPlane = m_nearPlane,
                            .farPlane = m_farPlane,
                            .orthographicSize = m_orthographicSize};
  stream.write(&version, sizeof(version));
  stream.write(&data, sizeof(data));
}

/*
 */
bool
CameraComponent::deserialize(DataStream& stream)
{
  uint32 version = 0;
  CameraFileData data;
  if (stream.read(&version, sizeof(version)) != sizeof(version) || version == 0 ||
      version > SERIALIZATION_VERSION || stream.read(&data, sizeof(data)) != sizeof(data) ||
      data.projectionType >= static_cast<uint32>(CameraProjectionType::COUNT)) {
    return false;
  }

  setProjectionType(static_cast<CameraProjectionType>(data.projectionType));
  setFieldOfView(Degree(data.fieldOfViewDegrees));
  setClipPlanes(data.nearPlane, data.farPlane);
  setOrthographicSize(data.orthographicSize);
  return true;
}

/*
 */
void
CameraComponent::onRegister()
{
  getOwner()->getScene()->addCamera(*this);
}

/*
 */
void
CameraComponent::onUnregister()
{
  getOwner()->getScene()->removeCamera(*this);
}

} // namespace chEngineSDK
