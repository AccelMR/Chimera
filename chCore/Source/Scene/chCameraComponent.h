/************************************************************************/
/**
 * @file chCameraComponent.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Component that a game views the scene through.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAngle.h"
#include "chCamera.h"
#include "chComponent.h"
#include "chTypeTraits.h"

namespace chEngineSDK {

/**
 * Exists so a scene saves the point of view a game shows it from. It looks along its
 * GameObject's forward axis from the object's position; roll is dropped and the pitch stops
 * short of straight up or down, as with Camera. While registered the scene knows it; when
 * the scene has several, the last one registered is used.
 */
class CH_CORE_EXPORT CameraComponent : public Component
{
 public:
  CameraComponent() = default;

  void
  setProjectionType(CameraProjectionType type);

  NODISCARD FORCEINLINE CameraProjectionType
  getProjectionType() const noexcept { return m_projectionType; }

  /**
   * The horizontal field of view of the perspective projection, kept between 1 and 170
   * degrees.
   */
  void
  setFieldOfView(Degree fieldOfView);

  NODISCARD FORCEINLINE Degree
  getFieldOfView() const noexcept { return m_fieldOfView; }

  /**
   * The near plane is kept at 0.001 or more and the far plane at least twice as far.
   */
  void
  setClipPlanes(float nearPlane, float farPlane);

  NODISCARD FORCEINLINE float
  getNearClipPlane() const noexcept { return m_nearPlane; }

  NODISCARD FORCEINLINE float
  getFarClipPlane() const noexcept { return m_farPlane; }

  /**
   * Half the visible height of the orthographic projection, in world units.
   */
  void
  setOrthographicSize(float halfHeight);

  NODISCARD FORCEINLINE float
  getOrthographicSize() const noexcept { return m_orthographicSize; }

  /**
   * Writes the view and projection of this camera into a Camera for a viewport of this
   * size in pixels.
   */
  void
  fillCamera(Camera& outCamera, float viewportWidth, float viewportHeight) const;

  void
  serialize(DataStream& stream) const override;

  NODISCARD bool
  deserialize(DataStream& stream) override;

  static constexpr uint32 SERIALIZATION_VERSION = 1;

 protected:
  void
  onRegister() override;

  void
  onUnregister() override;

 private:
  CameraProjectionType m_projectionType = CameraProjectionType::Perspective;
  Degree m_fieldOfView = Degree(90.0f);
  float m_nearPlane = 0.1f;
  float m_farPlane = 10000.0f;
  float m_orthographicSize = 5.0f;
};

DECLARE_TYPE_TRAITS(CameraComponent)

} // namespace chEngineSDK
