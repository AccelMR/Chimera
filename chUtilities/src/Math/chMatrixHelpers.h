/************************************************************************/
/**
 * @file chMatrixHelpers.h
 * @author AccelMR
 * @date 2025/05/01
 *   Helper matrix classes that inherit from Matrix4
 *
 * Coordinate system being X = front, Z = up, Y = right
 */
/************************************************************************/
#pragma once

#include "chMatrix4.h"
#include "chRotator.h"
#include "chVector3.h"

namespace chEngineSDK {

/************************************************************************/
/*
 * Translation Matrix
 */
/************************************************************************/
/**
 * @brief Translation Matrix class
 *
 * Sample usage:
 *  const TranslationMatrix posMatrix(Vector3(2.0f, 3.0f, 150.0f))
 */
class CH_UTILITY_EXPORT TranslationMatrix : public Matrix4
{
 public:
  /**
   * @brief Constructor for translation matrix given a vector
   *
   * @param translation Initial position for this Matrix to be
   */
  FORCEINLINE
  TranslationMatrix(const Vector3& translation)
   : Matrix4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
             translation.x, translation.y, translation.z, 1.0f) {}
};

/************************************************************************/
/*
 * Rotation Translation Matrix
 */
/************************************************************************/
/**
 * @brief Rotation Translation matrix class
 *
 * Sample usage:
 *   RotationTranslationMatrix RTM(Rotator(90.f,90.f,90.f), Vector3::UNITY);
 */
class CH_UTILITY_EXPORT RotationTranslationMatrix : public Matrix4
{
 public:
  /**
   * @brief Constructor base for Rotation-Translation matrix
   *
   * @param rotator Rotation component
   * @param origin Translation component
   */
  RotationTranslationMatrix(const Rotator& rotator, const Vector3& origin);
};

/************************************************************************/
/*
 * Rotation Matrix
 */
/************************************************************************/
/**
 * @brief Rotation Matrix class
 *
 * Sample usage:
 *   RotationMatrix RM(Rotator(90.f, 90.f, 90.f));
 */
class CH_UTILITY_EXPORT RotationMatrix : public RotationTranslationMatrix
{
 public:
  /**
   * @brief Constructor for pure rotation matrix
   *
   * @param rotator Rotation component
   */
  FORCEINLINE
  RotationMatrix(const Rotator& rotator) : RotationTranslationMatrix(rotator, Vector3::ZERO) {}
};

/************************************************************************/
/*
 * Scale Rotation Translation Matrix
 */
/************************************************************************/
/**
 * @brief Scale Rotation Translation Matrix
 *
 * Sample usage:
 *  ScaleRotationTranslationMatrix SRTM(Vector3(1.f,1.f,1.f),
 *                                      Rotator(90.f,90.f,90.f),
 *                                      Vector3::ZERO);
 */
class CH_UTILITY_EXPORT ScaleRotationTranslationMatrix : public Matrix4
{
 public:
  /**
   * @brief Constructor for Scale-Rotation-Translation matrix
   *
   * @param scale Scale component
   * @param rotator Rotation component
   * @param origin Translation component
   */
  ScaleRotationTranslationMatrix(const Vector3& scale, const Rotator& rotator,
                                 const Vector3& origin);
};

/************************************************************************/
/*
 * Perspective Matrix
 */
/************************************************************************/
/**
 * @brief Perspective projection matrix class
 */
class CH_UTILITY_EXPORT PerspectiveMatrix : public Matrix4
{
 public:
  /**
   * @brief Constructor for perspective projection matrix
   *
   * @param halfFOV Half Field of View in the Y direction
   * @param width View space width
   * @param height View space height
   * @param near Distance to the near Z plane
   * @param far Distance to the far Z plane
   */
  PerspectiveMatrix(const Radian& halfFOV, float width, float height, float near, float far);
};

/************************************************************************/
/*
 * Look At Matrix
 */
/************************************************************************/
/**
 * @brief Look at matrix class for view transformations
 */
class CH_UTILITY_EXPORT LookAtMatrix : public Matrix4
{
 public:
  /**
   * @brief Creates a view matrix using an eye position, a look at position, and up vector
   * This does the same thing as D3DXMatrixLookAtLH
   *
   * @param eyePosition Position of the camera
   * @param lookAtPosition Point the camera is looking at
   * @param upVector Up vector for orientation
   */
  LookAtMatrix(const Vector3& eyePosition, const Vector3& lookAtPosition,
               const Vector3& upVector);
};

} // namespace chEngineSDK
