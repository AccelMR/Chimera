/************************************************************************/
/**
 * @file chShapeOverlap.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Overlap tests between points, boxes, planes, spheres, capsules and frustums.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chBox.h"
#include "chCapsule.h"
#include "chFrustum.h"
#include "chOrientedBox.h"
#include "chPlane.h"
#include "chSphere.h"
#include "chSphereBoxBounds.h"

namespace chEngineSDK {
/**
 * Gives culling and picking one set of overlap tests. Everything is inline and compares
 * squared distances, so no test calls sqrt. Touching counts as overlapping in every test.
 * Plane normals must be unit length.
 */
class ShapeOverlap
{
 public:
  ShapeOverlap() = delete;

  /************************************************************************/
  /*
   * Axis-aligned boxes.
   */
  /************************************************************************/
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
   * True when the plane crosses the box.
   */
  NODISCARD static FORCEINLINE bool
  boxPlane(const AABox& box, const Plane& plane) noexcept
  {
    const Vector3 center = box.getCenter();
    return Math::abs(plane.planeDot(center)) <=
           projectedRadius(box.maxPoint - center, plane.normal);
  }

  NODISCARD static FORCEINLINE bool
  boxSphere(const AABox& box, const Sphere& sphere) noexcept
  {
    const Vector3 closest(Math::clamp(sphere.center.x, box.minPoint.x, box.maxPoint.x),
                          Math::clamp(sphere.center.y, box.minPoint.y, box.maxPoint.y),
                          Math::clamp(sphere.center.z, box.minPoint.z, box.maxPoint.z));
    return pointSphere(closest, sphere);
  }

  /************************************************************************/
  /*
   * Spheres.
   */
  /************************************************************************/
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

  /**
   * True when the plane crosses the sphere.
   */
  NODISCARD static FORCEINLINE bool
  spherePlane(const Sphere& sphere, const Plane& plane) noexcept
  {
    return Math::abs(plane.planeDot(sphere.center)) <= sphere.radius;
  }

  /************************************************************************/
  /*
   * Oriented boxes.
   */
  /************************************************************************/
  NODISCARD static FORCEINLINE bool
  pointOrientedBox(const Vector3& point, const OrientedBox& box) noexcept
  {
    const Vector3 local = box.toLocalPoint(point);
    return Math::abs(local.x) <= box.extent.x && Math::abs(local.y) <= box.extent.y &&
           Math::abs(local.z) <= box.extent.z;
  }

  NODISCARD static FORCEINLINE bool
  sphereOrientedBox(const Sphere& sphere, const OrientedBox& box) noexcept
  {
    return pointSphere(box.getClosestPoint(sphere.center), sphere);
  }

  NODISCARD static FORCEINLINE bool
  boxOrientedBox(const AABox& a, const OrientedBox& b) noexcept
  {
    return orientedBoxOrientedBox(OrientedBox(a.getCenter(), a.getExtent(),
                                              Quaternion::IDENTITY),
                                  b);
  }

  /**
   * Separating axis test over the 15 axes that can split two boxes: the 3 axes of each
   * box and the 9 cross products of one axis from each (Ericson, Real-Time Collision
   * Detection, 4.4.1).
   */
  NODISCARD static bool
  orientedBoxOrientedBox(const OrientedBox& a, const OrientedBox& b) noexcept
  {
    const Vector3 axesA[3] = {a.getAxis(0), a.getAxis(1), a.getAxis(2)};
    const Vector3 axesB[3] = {b.getAxis(0), b.getAxis(1), b.getAxis(2)};
    const float ea[3] = {a.extent.x, a.extent.y, a.extent.z};
    const float eb[3] = {b.extent.x, b.extent.y, b.extent.z};

    // b's axes in a's space, and the offset between the centers in a's space. The small
    // value added to the absolute values keeps the cross product axes from giving false
    // results when two edges are almost parallel (their cross product is close to zero).
    float r[3][3];
    float absR[3][3];
    for (int32 i = 0; i < 3; ++i) {
      for (int32 j = 0; j < 3; ++j) {
        r[i][j] = axesA[i].dot(axesB[j]);
        absR[i][j] = Math::abs(r[i][j]) + Math::SMALL_NUMBER;
      }
    }
    const Vector3 offset = b.center - a.center;
    const float t[3] = {offset.dot(axesA[0]), offset.dot(axesA[1]), offset.dot(axesA[2])};

    // Axes of a.
    for (int32 i = 0; i < 3; ++i) {
      const float rb = eb[0] * absR[i][0] + eb[1] * absR[i][1] + eb[2] * absR[i][2];
      if (Math::abs(t[i]) > ea[i] + rb) {
        return false;
      }
    }

    // Axes of b.
    for (int32 j = 0; j < 3; ++j) {
      const float ra = ea[0] * absR[0][j] + ea[1] * absR[1][j] + ea[2] * absR[2][j];
      if (Math::abs(t[0] * r[0][j] + t[1] * r[1][j] + t[2] * r[2][j]) > ra + eb[j]) {
        return false;
      }
    }

    // Cross products axesA[i] x axesB[j].
    for (int32 i = 0; i < 3; ++i) {
      const int32 i1 = (i + 1) % 3;
      const int32 i2 = (i + 2) % 3;
      for (int32 j = 0; j < 3; ++j) {
        const int32 j1 = (j + 1) % 3;
        const int32 j2 = (j + 2) % 3;
        const float ra = ea[i1] * absR[i2][j] + ea[i2] * absR[i1][j];
        const float rb = eb[j1] * absR[i][j2] + eb[j2] * absR[i][j1];
        if (Math::abs(t[i2] * r[i1][j] - t[i1] * r[i2][j]) > ra + rb) {
          return false;
        }
      }
    }

    return true;
  }

  /************************************************************************/
  /*
   * Capsules.
   */
  /************************************************************************/
  NODISCARD static FORCEINLINE bool
  pointCapsule(const Vector3& point, const Capsule& capsule) noexcept
  {
    return point.sqrDistance(capsule.getClosestAxisPoint(point)) <=
           Math::square(capsule.radius);
  }

  NODISCARD static FORCEINLINE bool
  sphereCapsule(const Sphere& sphere, const Capsule& capsule) noexcept
  {
    return sphere.center.sqrDistance(capsule.getClosestAxisPoint(sphere.center)) <=
           Math::square(sphere.radius + capsule.radius);
  }

  NODISCARD static FORCEINLINE bool
  capsuleCapsule(const Capsule& a, const Capsule& b) noexcept
  {
    return sqrDistanceSegmentSegment(a.start, a.end, b.start, b.end) <=
           Math::square(a.radius + b.radius);
  }

  /**
   * True when the plane crosses the capsule.
   */
  NODISCARD static FORCEINLINE bool
  capsulePlane(const Capsule& capsule, const Plane& plane) noexcept
  {
    const float startDistance = plane.planeDot(capsule.start);
    const float endDistance = plane.planeDot(capsule.end);

    // The ends on opposite sides: the axis itself crosses the plane.
    if ((startDistance < 0.0f) != (endDistance < 0.0f)) {
      return true;
    }
    return Math::min(Math::abs(startDistance), Math::abs(endDistance)) <= capsule.radius;
  }

  /************************************************************************/
  /*
   * Frustums. Bounds that cross a corner of the frustum outside every plane can still
   * be reported inside; culling accepts that to stay cheap.
   */
  /************************************************************************/
  NODISCARD static FORCEINLINE bool
  frustumPoint(const Frustum& frustum, const Vector3& point) noexcept
  {
    for (const Plane& plane : frustum.planes) {
      if (plane.planeDot(point) < 0.0f) {
        return false;
      }
    }
    return true;
  }

  NODISCARD static FORCEINLINE bool
  frustumSphere(const Frustum& frustum, const Sphere& sphere) noexcept
  {
    for (const Plane& plane : frustum.planes) {
      if (plane.planeDot(sphere.center) < -sphere.radius) {
        return false;
      }
    }
    return true;
  }

  NODISCARD static FORCEINLINE bool
  frustumBox(const Frustum& frustum, const AABox& box) noexcept
  {
    const Vector3 center = box.getCenter();
    const Vector3 extent = box.maxPoint - center;
    for (const Plane& plane : frustum.planes) {
      if (plane.planeDot(center) < -projectedRadius(extent, plane.normal)) {
        return false;
      }
    }
    return true;
  }

  /**
   * Both shapes hold everything inside the bounds, so it is outside as soon as either one
   * is fully behind a plane; the box is only measured when the sphere crosses the plane.
   */
  NODISCARD static FORCEINLINE bool
  frustumBounds(const Frustum& frustum, const SphereBoxBounds& bounds) noexcept
  {
    for (const Plane& plane : frustum.planes) {
      const float distance = plane.planeDot(bounds.center);
      if (distance >= bounds.sphereRadius) {
        continue;
      }
      if (distance < -bounds.sphereRadius ||
          distance < -projectedRadius(bounds.boxExtent, plane.normal)) {
        return false;
      }
    }
    return true;
  }

 private:
  // Half the size of a box with this extent, measured along a unit normal.
  NODISCARD static FORCEINLINE constexpr float
  projectedRadius(const Vector3& extent, const Vector3& normal) noexcept
  {
    return extent.x * Math::abs(normal.x) + extent.y * Math::abs(normal.y) +
           extent.z * Math::abs(normal.z);
  }

  // Squared distance between the closest points of segments p1-q1 and p2-q2 (Ericson,
  // Real-Time Collision Detection, 5.1.9).
  NODISCARD static constexpr float
  sqrDistanceSegmentSegment(const Vector3& p1, const Vector3& q1, const Vector3& p2,
                            const Vector3& q2) noexcept
  {
    const Vector3 d1 = q1 - p1;
    const Vector3 d2 = q2 - p2;
    const Vector3 r = p1 - p2;
    const float a = d1.dot(d1);
    const float e = d2.dot(d2);
    const float f = d2.dot(r);

    // s and t say where the closest points are along each segment, from 0 to 1.
    float s = 0.0f;
    float t = 0.0f;
    if (a <= Math::SMALL_NUMBER && e <= Math::SMALL_NUMBER) {
      return r.dot(r);
    }
    if (a <= Math::SMALL_NUMBER) {
      t = Math::clamp(f / e, 0.0f, 1.0f);
    }
    else {
      const float c = d1.dot(r);
      if (e <= Math::SMALL_NUMBER) {
        s = Math::clamp(-c / a, 0.0f, 1.0f);
      }
      else {
        const float b = d1.dot(d2);
        const float denominator = a * e - b * b;

        // Parallel segments: any s works, so start from 0.
        if (denominator != 0.0f) {
          s = Math::clamp((b * f - c * e) / denominator, 0.0f, 1.0f);
        }
        t = (b * s + f) / e;
        if (t < 0.0f) {
          t = 0.0f;
          s = Math::clamp(-c / a, 0.0f, 1.0f);
        }
        else if (t > 1.0f) {
          t = 1.0f;
          s = Math::clamp((b - c) / a, 0.0f, 1.0f);
        }
      }
    }

    const Vector3 closest1 = p1 + d1 * s;
    const Vector3 closest2 = p2 + d2 * t;
    return closest1.sqrDistance(closest2);
  }
};
} // namespace chEngineSDK
