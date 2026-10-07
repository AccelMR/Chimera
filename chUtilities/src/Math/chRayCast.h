/************************************************************************/
/**
 * @file chRayCast.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Where a ray hits points, boxes, spheres, planes and triangles.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chBox.h"
#include "chMath.h"
#include "chOrientedBox.h"
#include "chPlane.h"
#include "chRay.h"
#include "chSphere.h"

namespace chEngineSDK {
/**
 * Gives picking and line of sight one set of ray tests. Unlike ShapeOverlap, each test
 * also says where the ray hits: it returns true and writes outDistance, the distance
 * along the ray (ray.getPoint(outDistance) is the hit point). A ray that starts inside a
 * solid shape hits it at distance 0. Everything is inline and nothing allocates.
 */
class RayCast
{
 public:
  RayCast() = delete;

  NODISCARD static FORCEINLINE bool
  box(const Ray& ray, const AABox& shape, float& outDistance) noexcept;

  NODISCARD static FORCEINLINE bool
  orientedBox(const Ray& ray, const OrientedBox& shape, float& outDistance) noexcept;

  NODISCARD static FORCEINLINE bool
  sphere(const Ray& ray, const Sphere& shape, float& outDistance) noexcept;

  /**
   * Hits the plane from either side; a ray parallel to it never hits.
   */
  NODISCARD static FORCEINLINE bool
  plane(const Ray& ray, const Plane& shape, float& outDistance) noexcept;

  /**
   * Hits the triangle from either side (Moller-Trumbore).
   */
  NODISCARD static FORCEINLINE bool
  triangle(const Ray& ray, const Vector3& a, const Vector3& b, const Vector3& c,
           float& outDistance) noexcept;

 private:
  // Slab test of a ray against the box from minPoint to maxPoint, given per axis.
  NODISCARD static FORCEINLINE bool
  slabs(const float origin[3], const float direction[3], const float minPoint[3],
        const float maxPoint[3], float& outDistance) noexcept;

  // Below this a direction component or a determinant counts as zero.
  static constexpr float PARALLEL_EPSILON = 1.e-8f;
};

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
FORCEINLINE bool
RayCast::slabs(const float origin[3], const float direction[3], const float minPoint[3],
               const float maxPoint[3], float& outDistance) noexcept
{
  float nearest = 0.0f;
  float farthest = 3.402823466e+38f;
  for (int32 axis = 0; axis < 3; ++axis) {
    if (Math::abs(direction[axis]) < PARALLEL_EPSILON) {
      // Parallel to these two faces: it misses unless it already runs between them.
      if (origin[axis] < minPoint[axis] || origin[axis] > maxPoint[axis]) {
        return false;
      }
      continue;
    }

    const float inverse = 1.0f / direction[axis];
    float entry = (minPoint[axis] - origin[axis]) * inverse;
    float exit = (maxPoint[axis] - origin[axis]) * inverse;
    if (entry > exit) {
      const float swap = entry;
      entry = exit;
      exit = swap;
    }
    nearest = Math::max(nearest, entry);
    farthest = Math::min(farthest, exit);
    if (nearest > farthest) {
      return false;
    }
  }

  outDistance = nearest;
  return true;
}

/*
 */
FORCEINLINE bool
RayCast::box(const Ray& ray, const AABox& shape, float& outDistance) noexcept
{
  const float origin[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
  const float direction[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
  const float minPoint[3] = {shape.minPoint.x, shape.minPoint.y, shape.minPoint.z};
  const float maxPoint[3] = {shape.maxPoint.x, shape.maxPoint.y, shape.maxPoint.z};
  return slabs(origin, direction, minPoint, maxPoint, outDistance);
}

/*
 */
FORCEINLINE bool
RayCast::orientedBox(const Ray& ray, const OrientedBox& shape, float& outDistance) noexcept
{
  // In the box's space it is a box aligned with the axes; a rotation keeps distances, so
  // the distance found there is the distance in world space.
  const Vector3 localOrigin = shape.toLocalPoint(ray.origin);
  const Vector3 localDirection = shape.toLocalDirection(ray.direction);
  const float origin[3] = {localOrigin.x, localOrigin.y, localOrigin.z};
  const float direction[3] = {localDirection.x, localDirection.y, localDirection.z};
  const float minPoint[3] = {-shape.extent.x, -shape.extent.y, -shape.extent.z};
  const float maxPoint[3] = {shape.extent.x, shape.extent.y, shape.extent.z};
  return slabs(origin, direction, minPoint, maxPoint, outDistance);
}

/*
 */
FORCEINLINE bool
RayCast::sphere(const Ray& ray, const Sphere& shape, float& outDistance) noexcept
{
  const Vector3 offset = ray.origin - shape.center;
  const float along = offset.dot(ray.direction);
  const float outside = offset.dot(offset) - Math::square(shape.radius);

  // Starts outside and points away.
  if (outside > 0.0f && along > 0.0f) {
    return false;
  }

  const float discriminant = Math::square(along) - outside;
  if (discriminant < 0.0f) {
    return false;
  }

  outDistance = Math::max(0.0f, -along - Math::sqrt(discriminant));
  return true;
}

/*
 */
FORCEINLINE bool
RayCast::plane(const Ray& ray, const Plane& shape, float& outDistance) noexcept
{
  const float facing = shape.normal.dot(ray.direction);
  if (Math::abs(facing) < PARALLEL_EPSILON) {
    return false;
  }

  const float distance = -shape.planeDot(ray.origin) / facing;
  if (distance < 0.0f) {
    return false;
  }

  outDistance = distance;
  return true;
}

/*
 */
FORCEINLINE bool
RayCast::triangle(const Ray& ray, const Vector3& a, const Vector3& b, const Vector3& c,
                  float& outDistance) noexcept
{
  const Vector3 edge1 = b - a;
  const Vector3 edge2 = c - a;
  const Vector3 p = ray.direction.cross(edge2);
  const float determinant = edge1.dot(p);
  if (Math::abs(determinant) < PARALLEL_EPSILON) {
    return false;
  }

  // u and v are the barycentric coordinates of the hit inside the triangle.
  const float inverse = 1.0f / determinant;
  const Vector3 toOrigin = ray.origin - a;
  const float u = toOrigin.dot(p) * inverse;
  if (u < 0.0f || u > 1.0f) {
    return false;
  }

  const Vector3 q = toOrigin.cross(edge1);
  const float v = ray.direction.dot(q) * inverse;
  if (v < 0.0f || u + v > 1.0f) {
    return false;
  }

  const float distance = edge2.dot(q) * inverse;
  if (distance < 0.0f) {
    return false;
  }

  outDistance = distance;
  return true;
}
} // namespace chEngineSDK
