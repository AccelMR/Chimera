/************************************************************************/
/**
 * @file chMatrixHelpers.h
 * @author AccelMR
 * @date 2025/05/01
 * @brief Matrix4 subclasses that build common transforms in their constructor.
 *
 * Coordinate system: X = forward, Y = right, Z = up, left-handed.
 */
/************************************************************************/
#pragma once

#include "chMatrix4.h"
#include "chRotator.h"
#include "chVector3.h"

namespace chEngineSDK {
class Quaternion;

/**
 * Builds a translation, so code reads TranslationMatrix(position) instead of filling 16
 * values by hand.
 */
class TranslationMatrix : public Matrix4
{
 public:
  FORCEINLINE explicit
  TranslationMatrix(const Vector3& translation) noexcept
   : Matrix4(1.0f, 0.0f, 0.0f, 0.0f,
             0.0f, 1.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 1.0f, 0.0f,
             translation.x, translation.y, translation.z, 1.0f)
  {}
};

/**
 * Builds a rotation followed by a translation from a Rotator, the same way Unreal does,
 * so the rows are the rotated forward, right and up axes.
 */
class RotationTranslationMatrix : public Matrix4
{
 public:
  CH_UTILITY_EXPORT
  RotationTranslationMatrix(const Rotator& rotator, const Vector3& origin) noexcept;
};

/**
 * Builds a rotation with no translation from a Rotator.
 */
class RotationMatrix : public RotationTranslationMatrix
{
 public:
  FORCEINLINE explicit
  RotationMatrix(const Rotator& rotator) noexcept
   : RotationTranslationMatrix(rotator, Vector3::ZERO)
  {}
};

/**
 * Builds scale, then rotation, then translation in one step, which is cheaper than
 * multiplying three matrices.
 */
class ScaleRotationTranslationMatrix : public Matrix4
{
 public:
  CH_UTILITY_EXPORT
  ScaleRotationTranslationMatrix(const Vector3& scale, const Rotator& rotator,
                                 const Vector3& origin) noexcept;

  /**
   * The rotation must have unit length.
   */
  CH_UTILITY_EXPORT
  ScaleRotationTranslationMatrix(const Vector3& scale, const Quaternion& rotation,
                                 const Vector3& origin) noexcept;
};

/**
 * Builds a perspective projection into Direct3D's clip space: X right, Y up, depth in
 * [0, 1]. The Vulkan backend flips Y with its viewport, so this stays API-agnostic.
 * halfFOV is half of the horizontal field of view, so changing the window height keeps
 * the visible width.
 */
class PerspectiveMatrix : public Matrix4
{
 public:
  CH_UTILITY_EXPORT
  PerspectiveMatrix(const Radian& halfFOV, float width, float height, float near,
                    float far) noexcept;
};

/**
 * Builds an orthographic projection into the same clip space as PerspectiveMatrix.
 */
class OrthographicMatrix : public Matrix4
{
 public:
  FORCEINLINE
  OrthographicMatrix(float halfWidth, float halfHeight, float near, float far) noexcept
   : Matrix4(1.0f / halfWidth, 0.0f, 0.0f, 0.0f,
             0.0f, 1.0f / halfHeight, 0.0f, 0.0f,
             0.0f, 0.0f, 1.0f / (far - near), 0.0f,
             0.0f, 0.0f, -near / (far - near), 1.0f)
  {}
};

/**
 * Builds a view matrix from the camera position, the point it looks at and its up vector,
 * like D3DXMatrixLookAtLH: in view space X is right, Y is up and Z is forward.
 */
class LookAtMatrix : public Matrix4
{
 public:
  CH_UTILITY_EXPORT
  LookAtMatrix(const Vector3& eyePosition, const Vector3& lookAtPosition,
               const Vector3& upVector) noexcept;
};

} // namespace chEngineSDK
