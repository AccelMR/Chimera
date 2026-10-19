/************************************************************************/
/**
 * @file chDebugDraw.cpp
 * @author AccelMR
 * @date 2026/10/18
 * @brief
 *  Lines drawn over the scene to show shapes, bounds and directions.
 */
/************************************************************************/
#include "chDebugDraw.h"

#include "chBox.h"
#include "chCapsule.h"
#include "chLinearColor.h"
#include "chLogger.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chOrientedBox.h"
#include "chRay.h"
#include "chRenderSettings.h"
#include "chSphere.h"
#include "chVector4.h"

#if USING(CH_DEBUG_DRAW)
namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(DebugDrawLog, All);

namespace {
NODISCARD uint32
packColor(const LinearColor& color)
{
  const auto toByte = [](float value) {
    return static_cast<uint32>(Math::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
  };
  return toByte(color.r) | (toByte(color.g) << 8) | (toByte(color.b) << 16) |
         (toByte(color.a) << 24);
}

/*
 * Two unit axes perpendicular to normal and to each other.
 */
void
makeBasis(const Vector3& normal, Vector3& outAxisA, Vector3& outAxisB)
{
  // The world axis least aligned with the normal gives the most precise cross product.
  const Vector3 helper = Math::abs(normal.z) < 0.9f ? Vector3::UP : Vector3::FORWARD;
  outAxisA = helper.cross(normal).getNormalized();
  outAxisB = normal.cross(outAxisA);
}

NODISCARD Vector3
toVector3(const Vector4& vector)
{
  return Vector3(vector.x, vector.y, vector.z);
}
} // namespace

/*
 */
void
DebugDraw::line(const Vector3& start,
                const Vector3& end,
                const LinearColor& color,
                DebugDepth depth)
{
  addLine(start, end, packColor(color), depth);
}

/*
 */
void
DebugDraw::ray(const Ray& ray, float length, const LinearColor& color, DebugDepth depth)
{
  addLine(ray.origin, ray.origin + ray.direction * length, packColor(color), depth);
}

/*
 */
void
DebugDraw::box(const AABox& box, const LinearColor& color, DebugDepth depth)
{
  Vector3 corners[8];
  for (uint32 i = 0; i < 8; ++i) {
    corners[i] = Vector3((i & 1) ? box.maxPoint.x : box.minPoint.x,
                         (i & 2) ? box.maxPoint.y : box.minPoint.y,
                         (i & 4) ? box.maxPoint.z : box.minPoint.z);
  }
  addBoxEdges(corners, packColor(color), depth);
}

/*
 */
void
DebugDraw::box(const AABox& localBox,
               const Matrix4& transform,
               const LinearColor& color,
               DebugDepth depth)
{
  Vector3 corners[8];
  for (uint32 i = 0; i < 8; ++i) {
    const Vector3 local((i & 1) ? localBox.maxPoint.x : localBox.minPoint.x,
                        (i & 2) ? localBox.maxPoint.y : localBox.minPoint.y,
                        (i & 4) ? localBox.maxPoint.z : localBox.minPoint.z);
    corners[i] = toVector3(transform.transformPosition(local));
  }
  addBoxEdges(corners, packColor(color), depth);
}

/*
 */
void
DebugDraw::orientedBox(const OrientedBox& box, const LinearColor& color, DebugDepth depth)
{
  Vector3 corners[8];
  for (uint32 i = 0; i < 8; ++i) {
    const Vector3 local((i & 1) ? box.extent.x : -box.extent.x,
                        (i & 2) ? box.extent.y : -box.extent.y,
                        (i & 4) ? box.extent.z : -box.extent.z);
    corners[i] = box.center + box.rotation.rotateVector(local);
  }
  addBoxEdges(corners, packColor(color), depth);
}

/*
 */
void
DebugDraw::circle(const Vector3& center,
                  const Vector3& normal,
                  float radius,
                  const LinearColor& color,
                  DebugDepth depth,
                  uint32 segments)
{
  const Vector3 unitNormal = normal.getNormalized();
  if (unitNormal == Vector3::ZERO) {
    return;
  }
  Vector3 axisA;
  Vector3 axisB;
  makeBasis(unitNormal, axisA, axisB);
  arc(center, axisA, axisB, radius, 0.0f, Math::TWO_PI, segments, packColor(color), depth);
}

/*
 */
void
DebugDraw::sphere(const Sphere& sphere,
                  const LinearColor& color,
                  DebugDepth depth,
                  uint32 segments)
{
  const uint32 packed = packColor(color);
  arc(sphere.center, Vector3::FORWARD, Vector3::RIGHT, sphere.radius, 0.0f, Math::TWO_PI,
      segments, packed, depth);
  arc(sphere.center, Vector3::FORWARD, Vector3::UP, sphere.radius, 0.0f, Math::TWO_PI,
      segments, packed, depth);
  arc(sphere.center, Vector3::RIGHT, Vector3::UP, sphere.radius, 0.0f, Math::TWO_PI,
      segments, packed, depth);
}

/*
 */
void
DebugDraw::capsule(const Capsule& capsule,
                   const LinearColor& color,
                   DebugDepth depth,
                   uint32 segments)
{
  const Vector3 axis = capsule.end - capsule.start;
  const Vector3 direction = axis.getNormalized();
  if (direction == Vector3::ZERO) {
    sphere(Sphere(capsule.start, capsule.radius), color, depth, segments);
    return;
  }

  const uint32 packed = packColor(color);
  const float radius = capsule.radius;
  Vector3 axisA;
  Vector3 axisB;
  makeBasis(direction, axisA, axisB);
  arc(capsule.start, axisA, axisB, radius, 0.0f, Math::TWO_PI, segments, packed, depth);
  arc(capsule.end, axisA, axisB, radius, 0.0f, Math::TWO_PI, segments, packed, depth);

  const uint32 halfSegments = Math::max(segments / 2, 1u);
  for (const Vector3& side : {axisA, axisB}) {
    addLine(capsule.start + side * radius, capsule.end + side * radius, packed, depth);
    addLine(capsule.start - side * radius, capsule.end - side * radius, packed, depth);
    arc(capsule.end, side, direction, radius, 0.0f, Math::PI, halfSegments, packed, depth);
    arc(capsule.start, side, -direction, radius, 0.0f, Math::PI, halfSegments, packed,
        depth);
  }
}

/*
 */
void
DebugDraw::frustum(const Matrix4& viewProjection, const LinearColor& color, DebugDepth depth)
{
  // Clip space corners: x and y from -1 to 1, depth from 0 to 1.
  const Matrix4 inverse = viewProjection.getInverse();
  Vector3 corners[8];
  for (uint32 i = 0; i < 8; ++i) {
    const Vector4 clip((i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f, (i & 4) ? 1.0f : 0.0f,
                       1.0f);
    const Vector4 world = inverse.transformVector4(clip);
    if (Math::abs(world.w) <= Math::SMALL_NUMBER) {
      return;
    }
    corners[i] = toVector3(world) / world.w;
  }
  addBoxEdges(corners, packColor(color), depth);
}

/*
 */
void
DebugDraw::axes(const Matrix4& transform, float size, DebugDepth depth)
{
  const Vector3 origin = toVector3(transform.transformPosition(Vector3::ZERO));
  const Vector3 worldAxes[] = {Vector3::FORWARD, Vector3::RIGHT, Vector3::UP};
  const uint32 colors[] = {packColor(LinearColor::Red), packColor(LinearColor::Green),
                           packColor(LinearColor::Blue)};
  for (uint32 i = 0; i < 3; ++i) {
    const Vector3 direction = toVector3(transform.transformVector(worldAxes[i]));
    const Vector3 unitDirection = direction.getNormalized();
    if (unitDirection != Vector3::ZERO) {
      addLine(origin, origin + unitDirection * size, colors[i], depth);
    }
  }
}

/*
 */
void
DebugDraw::clear() noexcept
{
  for (Vector<DebugLineVertex>& vertices : m_vertices) {
    vertices.clear();
  }
}

/*
 */
void
DebugDraw::arc(const Vector3& center,
               const Vector3& axisA,
               const Vector3& axisB,
               float radius,
               float startAngle,
               float endAngle,
               uint32 segments,
               uint32 color,
               DebugDepth depth)
{
  const uint32 count = Math::max(segments, 1u);
  const float step = (endAngle - startAngle) / static_cast<float>(count);
  const auto pointAt = [&](uint32 index) {
    float sine = 0.0f;
    float cosine = 0.0f;
    Math::sinCos(startAngle + step * static_cast<float>(index), sine, cosine);
    return center + (axisA * cosine + axisB * sine) * radius;
  };

  Vector3 previous = pointAt(0);
  for (uint32 i = 1; i <= count; ++i) {
    const Vector3 next = pointAt(i);
    addLine(previous, next, color, depth);
    previous = next;
  }
}

/*
 */
void
DebugDraw::addLine(const Vector3& start, const Vector3& end, uint32 color, DebugDepth depth)
{
  const uint32 maxLines = static_cast<uint32>(Math::max(g_cvarDebugLinesMax.get(), 1));
  if (getLineCount() >= maxLines) {
    if (!m_bWarnedFull) {
      m_bWarnedFull = true;
      CH_LOG_WARNING(DebugDrawLog,
                     "More than {0} debug lines between two clears; the rest are dropped "
                     "(Renderer.DebugLinesMax).",
                     maxLines);
    }
    return;
  }

  Vector<DebugLineVertex>& vertices = m_vertices[static_cast<uint32>(depth)];
  vertices.push_back({.position = start, .color = color});
  vertices.push_back({.position = end, .color = color});
}

/*
 */
void
DebugDraw::addBoxEdges(const Vector3 (&corners)[8], uint32 color, DebugDepth depth)
{
  for (uint32 corner = 0; corner < 8; ++corner) {
    for (uint32 bit = 1; bit < 8; bit <<= 1) {
      if ((corner & bit) == 0) {
        addLine(corners[corner], corners[corner | bit], color, depth);
      }
    }
  }
}

} // namespace chEngineSDK
#endif // USING(CH_DEBUG_DRAW)
