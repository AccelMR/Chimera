/************************************************************************/
/**
 * @file chShapeOverlap.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Overlap tests between points, boxes, planes and spheres.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chBox.h"
#include "chPlane.h"
#include "chSphere.h"
#include "chSphereBoxBounds.h"

namespace chEngineSDK {
/**
 * Gives culling and picking one set of overlap tests. Everything is inline and compares
 * squared distances, so no test calls sqrt. Touching counts as overlapping in every test.
 */
class ShapeOverlap
{
 public:
  ShapeOverlap() = delete;

  NODISCARD static FORCEINLINE bool
  pointBox(const Vector3& point, const AABox& box) noexcept
  {
    return point.x >= box.minPoint.x && point.x <= box.maxPoint.x &&
           point.y >= box.minPoint.y && point.y <= box.maxPoint.y &&
           point.z >= box.minPoint.z && point.z <= box.maxPoint.z;
  }

  NODISCARD static FORCEINLINE bool
  boxBox(const AABox& a, const AABox& b) noexcept
  {
    return a.minPoint.x <= b.maxPoint.x && b.minPoint.x <= a.maxPoint.x &&
           a.minPoint.y <= b.maxPoint.y && b.minPoint.y <= a.maxPoint.y &&
           a.minPoint.z <= b.maxPoint.z && b.minPoint.z <= a.maxPoint.z;
  }

  NODISCARD static FORCEINLINE bool
  boxBox(const SphereBoxBounds& a, const SphereBoxBounds& b) noexcept
  {
    return boxBox(a.getBox(), b.getBox());
  }

  /**
   * True when the plane crosses the box. The plane normal must be unit length.
   */
  NODISCARD static FORCEINLINE bool
  boxPlane(const AABox& box, const Plane& plane) noexcept
  {
    const Vector3 center = box.getCenter();
    const Vector3 extent = box.maxPoint - center;

    // Half the size of the box projected on the plane normal.
    const float radius = extent.x * Math::abs(plane.normal.x) +
                         extent.y * Math::abs(plane.normal.y) +
                         extent.z * Math::abs(plane.normal.z);

    return Math::abs(plane.planeDot(center)) <= radius;
  }

  NODISCARD static FORCEINLINE bool
  pointSphere(const Vector3& point, const Sphere& sphere) noexcept
  {
    const Vector3 offset = point - sphere.center;
    return offset.dot(offset) <= Math::square(sphere.radius);
  }

  NODISCARD static FORCEINLINE bool
  sphereSphere(const Sphere& a, const Sphere& b) noexcept
  {
    const Vector3 offset = a.center - b.center;
    return offset.dot(offset) <= Math::square(a.radius + b.radius);
  }

  NODISCARD static FORCEINLINE bool
  sphereSphere(const SphereBoxBounds& a, const SphereBoxBounds& b,
               float tolerance = Math::KINDA_SMALL_NUMBER) noexcept
  {
    const Vector3 offset = a.center - b.center;
    return offset.dot(offset) <= Math::square(a.sphereRadius + b.sphereRadius + tolerance);
  }

  NODISCARD static FORCEINLINE bool
  boxSphere(const AABox& box, const Sphere& sphere) noexcept
  {
    const Vector3 closest(Math::clamp(sphere.center.x, box.minPoint.x, box.maxPoint.x),
                          Math::clamp(sphere.center.y, box.minPoint.y, box.maxPoint.y),
                          Math::clamp(sphere.center.z, box.minPoint.z, box.maxPoint.z));
    return pointSphere(closest, sphere);
  }
};
} // namespace chEngineSDK
