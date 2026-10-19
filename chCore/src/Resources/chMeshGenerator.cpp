/************************************************************************/
/**
 * @file chMeshGenerator.cpp
 * @author AccelMR
 * @date 2026/10/18
 * @brief
 *  Builds the meshes of simple shapes from their sizes.
 */
/************************************************************************/
#include "chMeshGenerator.h"

#include <limits>

#include "chHash.h"
#include "chMath.h"
#include "chMesh.h"
#include "chVector2.h"
#include "chVector4.h"
#include "chVertexLayout.h"

namespace chEngineSDK {
namespace {
/*
 * One point of a shape's surface. imageUp points up the texture (towards smaller V); the
 * vertex keeps only its side of the normal, as the tangent's w.
 */
struct SurfacePoint
{
  Vector3 position;
  Vector3 normal;
  Vector2 texCoord;
  Vector3 tangent;
  Vector3 imageUp;
};

/*
 * Collects the vertices and triangles of one mesh.
 */
class MeshBuilder
{
 public:
  uint32
  addVertex(const SurfacePoint& point)
  {
    Vector3 tangent = point.tangent - point.normal * point.normal.dot(point.tangent);
    tangent.normalize();
    const float side = point.normal.cross(tangent).dot(point.imageUp) >= 0.0f ? 1.0f : -1.0f;
    m_vertices.push_back({.position = point.position,
                          .normal = point.normal,
                          .texCoord = point.texCoord,
                          .tangent = Vector4(tangent.x, tangent.y, tangent.z, side)});
    return static_cast<uint32>(m_vertices.size() - 1);
  }

  /*
   * Front faces are clockwise, so each triangle is turned to show its front on the side
   * its normals point to; the shapes then need no winding rules of their own. A triangle
   * with two corners on the same point (the pole of a sphere, the tip of a cone) is
   * dropped.
   */
  void
  addTriangle(uint32 a, uint32 b, uint32 c)
  {
    const Vector3& positionA = m_vertices[a].position;
    const Vector3 faceNormal =
        (m_vertices[b].position - positionA).cross(m_vertices[c].position - positionA);
    if (faceNormal.sqrMagnitude() == 0.0f) {
      return;
    }
    const Vector3 normals =
        m_vertices[a].normal + m_vertices[b].normal + m_vertices[c].normal;
    const bool bFlip = faceNormal.dot(normals) < 0.0f;
    m_indices.push_back(a);
    m_indices.push_back(bFlip ? c : b);
    m_indices.push_back(bFlip ? b : c);
  }

  /*
   * a, b, c, d go around the quad. When one edge is a single point, the triangle that
   * keeps the corner of column a is used, so a pole vertex placed in the middle of its
   * column shades the right side.
   */
  void
  addQuad(uint32 a, uint32 b, uint32 c, uint32 d)
  {
    if (m_vertices[a].position == m_vertices[b].position) {
      addTriangle(a, c, d);
    }
    else if (m_vertices[c].position == m_vertices[d].position) {
      addTriangle(a, b, d);
    }
    else {
      addTriangle(a, b, c);
      addTriangle(a, c, d);
    }
  }

  /*
   * rows by columns points from pointAt(row, column), with a quad between every four.
   */
  template<typename PointFunction>
  void
  addGrid(uint32 rows, uint32 columns, PointFunction&& pointAt)
  {
    const uint32 first = static_cast<uint32>(m_vertices.size());
    for (uint32 row = 0; row < rows; ++row) {
      for (uint32 column = 0; column < columns; ++column) {
        addVertex(pointAt(row, column));
      }
    }
    for (uint32 row = 0; row + 1 < rows; ++row) {
      for (uint32 column = 0; column + 1 < columns; ++column) {
        const uint32 topLeft = first + row * columns + column;
        const uint32 bottomLeft = topLeft + columns;
        addQuad(topLeft, topLeft + 1, bottomLeft + 1, bottomLeft);
      }
    }
  }

  void
  build(Mesh& outMesh) const
  {
    outMesh.setVertexData(m_vertices);
    if (m_vertices.size() <= std::numeric_limits<uint16>::max()) {
      Vector<uint16> indices;
      indices.reserve(m_indices.size());
      for (const uint32 index : m_indices) {
        indices.push_back(static_cast<uint16>(index));
      }
      outMesh.setIndexData(indices);
    }
    else {
      outMesh.setIndexData(m_indices);
    }
  }

 private:
  Vector<VertexStatic> m_vertices;
  Vector<uint32> m_indices;
};

/*
 * Point of the circle around Z for texture U. The angle runs clockwise seen from above, so
 * U grows to the right of a viewer outside the shape.
 */
struct AroundZ
{
  float x;
  float y;

  explicit AroundZ(float u)
  {
    Math::sinCos(-u * Math::TWO_PI, y, x);
  }

  NODISCARD Vector3
  getTangent() const
  {
    return Vector3(y, -x, 0.0f);
  }
};

/*
 * U of a column; a row that is a single point uses the middle of the column instead, so its
 * triangle gets the texture of its own column.
 */
NODISCARD float
getColumnU(uint32 column, uint32 segments, bool bSinglePoint)
{
  const float offset = bSinglePoint ? 0.5f : 0.0f;
  return (static_cast<float>(column) + offset) / static_cast<float>(segments);
}

/*
 * Point of a sphere of radius around center, at the latitude given by its sine and cosine
 * (from the top pole, so cosLatitude 1 is the top).
 */
NODISCARD SurfacePoint
getSpherePoint(const Vector3& center,
               float radius,
               float sinLatitude,
               float cosLatitude,
               float u,
               float v)
{
  const AroundZ around(u);
  const Vector3 normal(sinLatitude * around.x, sinLatitude * around.y, cosLatitude);
  return {.position = center + normal * radius,
          .normal = normal,
          .texCoord = Vector2(u, v),
          .tangent = around.getTangent(),
          .imageUp = Vector3(-cosLatitude * around.x, -cosLatitude * around.y, sinLatitude)};
}

/*
 * Flat disc at height z facing up (bTop) or down, mapped as seen from that side with +X up
 * the image.
 */
void
addDisc(MeshBuilder& builder, float radius, float z, uint32 segments, bool bTop)
{
  const Vector3 normal = bTop ? Vector3::UP : Vector3::DOWN;
  // Seen from below, world right is -Y.
  const Vector3 right = bTop ? Vector3::RIGHT : Vector3::LEFT;
  builder.addGrid(2, segments + 1, [&](uint32 row, uint32 column) {
    const AroundZ around(getColumnU(column, segments, false));
    const float ringRadius = row == 0 ? 0.0f : radius;
    const Vector3 position(around.x * ringRadius, around.y * ringRadius, z);
    const float u = 0.5f + position.dot(right) / (2.0f * radius);
    const float v = 0.5f - position.x / (2.0f * radius);
    return SurfacePoint{.position = position,
                        .normal = normal,
                        .texCoord = Vector2(u, v),
                        .tangent = right,
                        .imageUp = Vector3::FORWARD};
  });
}

void
buildPlane(const ShapeDesc& desc, MeshBuilder& builder)
{
  const uint32 points = desc.subdivisions + 1;
  const float step = 1.0f / static_cast<float>(desc.subdivisions);
  builder.addGrid(points, points, [&](uint32 row, uint32 column) {
    const float u = static_cast<float>(column) * step;
    const float v = static_cast<float>(row) * step;
    return SurfacePoint{
        .position = Vector3((0.5f - v) * desc.size.x, (u - 0.5f) * desc.size.y, 0.0f),
        .normal = Vector3::UP,
        .texCoord = Vector2(u, v),
        .tangent = Vector3::RIGHT,
        .imageUp = Vector3::FORWARD};
  });
}

void
buildBox(const ShapeDesc& desc, MeshBuilder& builder)
{
  struct Face
  {
    Vector3 normal;
    // The right and up of a viewer looking at the face from outside.
    Vector3 right;
    Vector3 up;
  };
  const Face faces[] = {{Vector3::FORWARD, Vector3::LEFT, Vector3::UP},
                        {Vector3::BACKWARD, Vector3::RIGHT, Vector3::UP},
                        {Vector3::RIGHT, Vector3::FORWARD, Vector3::UP},
                        {Vector3::LEFT, Vector3::BACKWARD, Vector3::UP},
                        {Vector3::UP, Vector3::RIGHT, Vector3::FORWARD},
                        {Vector3::DOWN, Vector3::LEFT, Vector3::FORWARD}};

  const Vector3 half = desc.size * 0.5f;
  const auto halfAlong = [&half](const Vector3& axis) {
    return Math::abs(axis.x) * half.x + Math::abs(axis.y) * half.y +
           Math::abs(axis.z) * half.z;
  };
  for (const Face& face : faces) {
    const Vector3 center = face.normal * halfAlong(face.normal);
    const Vector3 right = face.right * halfAlong(face.right);
    const Vector3 up = face.up * halfAlong(face.up);
    builder.addGrid(2, 2, [&](uint32 row, uint32 column) {
      const float u = static_cast<float>(column);
      const float v = static_cast<float>(row);
      return SurfacePoint{.position = center + right * (2.0f * u - 1.0f) +
                                      up * (1.0f - 2.0f * v),
                          .normal = face.normal,
                          .texCoord = Vector2(u, v),
                          .tangent = face.right,
                          .imageUp = face.up};
    });
  }
}

void
buildSphere(const ShapeDesc& desc, MeshBuilder& builder)
{
  builder.addGrid(desc.rings + 1, desc.segments + 1, [&](uint32 row, uint32 column) {
    const bool bPole = row == 0 || row == desc.rings;
    const float v = static_cast<float>(row) / static_cast<float>(desc.rings);
    float sinLatitude = 0.0f;
    float cosLatitude = 0.0f;
    Math::sinCos(v * Math::PI, sinLatitude, cosLatitude);
    if (bPole) {
      // Exact, so the pole is one point and its zero-area triangles are dropped.
      sinLatitude = 0.0f;
      cosLatitude = row == 0 ? 1.0f : -1.0f;
    }
    return getSpherePoint(Vector3::ZERO, desc.radius, sinLatitude, cosLatitude,
                          getColumnU(column, desc.segments, bPole), v);
  });
}

/*
 * A sphere cut at its equator, with the halves moved apart by the straight part. V follows
 * the length along the surface, so the texture is not stretched on the straight part.
 */
void
buildCapsule(const ShapeDesc& desc, MeshBuilder& builder)
{
  const float radius = desc.radius;
  const float halfStraight = desc.height * 0.5f - radius;
  const uint32 halfRings = Math::max(desc.rings / 2, 1u);
  const float totalLength = Math::PI * radius + 2.0f * halfStraight;
  builder.addGrid(2 * (halfRings + 1), desc.segments + 1, [&](uint32 row, uint32 column) {
    const bool bBottom = row > halfRings;
    const uint32 step = bBottom ? row - halfRings - 1 : row;
    const float latitude =
        (static_cast<float>(step) / static_cast<float>(halfRings) + (bBottom ? 1.0f : 0.0f)) *
        Math::HALF_PI;
    float sinLatitude = 0.0f;
    float cosLatitude = 0.0f;
    Math::sinCos(latitude, sinLatitude, cosLatitude);
    const bool bPole = row == 0 || row == 2 * halfRings + 1;
    if (bPole) {
      sinLatitude = 0.0f;
      cosLatitude = bBottom ? -1.0f : 1.0f;
    }
    else if (step == (bBottom ? 0u : halfRings)) {
      // Exact at the equator, so a capsule without a straight part has no gap or sliver.
      sinLatitude = 1.0f;
      cosLatitude = 0.0f;
    }
    const float length = latitude * radius + (bBottom ? 2.0f * halfStraight : 0.0f);
    const Vector3 center(0.0f, 0.0f, bBottom ? -halfStraight : halfStraight);
    return getSpherePoint(center, radius, sinLatitude, cosLatitude,
                          getColumnU(column, desc.segments, bPole), length / totalLength);
  });
}

void
buildCylinder(const ShapeDesc& desc, MeshBuilder& builder)
{
  const float halfHeight = desc.height * 0.5f;
  builder.addGrid(2, desc.segments + 1, [&](uint32 row, uint32 column) {
    const float u = getColumnU(column, desc.segments, false);
    const AroundZ around(u);
    const float v = static_cast<float>(row);
    return SurfacePoint{.position = Vector3(around.x * desc.radius, around.y * desc.radius,
                                            row == 0 ? halfHeight : -halfHeight),
                        .normal = Vector3(around.x, around.y, 0.0f),
                        .texCoord = Vector2(u, v),
                        .tangent = around.getTangent(),
                        .imageUp = Vector3::UP};
  });
  addDisc(builder, desc.radius, halfHeight, desc.segments, true);
  addDisc(builder, desc.radius, -halfHeight, desc.segments, false);
}

void
buildCone(const ShapeDesc& desc, MeshBuilder& builder)
{
  const float halfHeight = desc.height * 0.5f;
  const float slopeLength = Math::sqrt(desc.height * desc.height + desc.radius * desc.radius);
  builder.addGrid(2, desc.segments + 1, [&](uint32 row, uint32 column) {
    const bool bTip = row == 0;
    const float u = getColumnU(column, desc.segments, bTip);
    const AroundZ around(u);
    const float ringRadius = bTip ? 0.0f : desc.radius;
    return SurfacePoint{
        .position = Vector3(around.x * ringRadius, around.y * ringRadius,
                            bTip ? halfHeight : -halfHeight),
        .normal = Vector3(around.x * desc.height, around.y * desc.height, desc.radius) /
                  slopeLength,
        .texCoord = Vector2(u, static_cast<float>(row)),
        .tangent = around.getTangent(),
        .imageUp = Vector3(-around.x * desc.radius, -around.y * desc.radius, desc.height) /
                   slopeLength};
  });
  addDisc(builder, desc.radius, -halfHeight, desc.segments, false);
}
} // namespace

/*
 */
uint64
ShapeDesc::getHash() const
{
  uint64 hash = HashUtils::FNV_OFFSET_BASIS;
  hash = HashUtils::combine(hash, type);
  hash = HashUtils::combine(hash, size.x);
  hash = HashUtils::combine(hash, size.y);
  hash = HashUtils::combine(hash, size.z);
  hash = HashUtils::combine(hash, radius);
  hash = HashUtils::combine(hash, height);
  hash = HashUtils::combine(hash, segments);
  hash = HashUtils::combine(hash, rings);
  return HashUtils::combine(hash, subdivisions);
}

/*
 */
void
MeshGenerator::generate(const ShapeDesc& desc, Mesh& outMesh)
{
  const ShapeDesc clamped = getClamped(desc);
  MeshBuilder builder;
  switch (clamped.type) {
    case ShapeType::Plane:
      buildPlane(clamped, builder);
      break;
    case ShapeType::Box:
      buildBox(clamped, builder);
      break;
    case ShapeType::Sphere:
      buildSphere(clamped, builder);
      break;
    case ShapeType::Capsule:
      buildCapsule(clamped, builder);
      break;
    case ShapeType::Cylinder:
      buildCylinder(clamped, builder);
      break;
    case ShapeType::Cone:
      buildCone(clamped, builder);
      break;
    case ShapeType::COUNT:
      break;
  }
  builder.build(outMesh);
}

/*
 */
ShapeDesc
MeshGenerator::getClamped(const ShapeDesc& desc)
{
  ShapeDesc clamped = desc;
  if (static_cast<uint32>(clamped.type) >= static_cast<uint32>(ShapeType::COUNT)) {
    clamped.type = ShapeType::Box;
  }
  clamped.size = Vector3(Math::max(desc.size.x, MIN_SIZE), Math::max(desc.size.y, MIN_SIZE),
                         Math::max(desc.size.z, MIN_SIZE));
  clamped.radius = Math::max(desc.radius, MIN_SIZE);
  clamped.height = Math::max(desc.height, MIN_SIZE);
  if (clamped.type == ShapeType::Capsule) {
    // The two half spheres need the whole height at least.
    clamped.height = Math::max(clamped.height, 2.0f * clamped.radius);
  }
  clamped.segments = Math::clamp(desc.segments, MIN_SEGMENTS, MAX_SEGMENTS);
  clamped.rings = Math::clamp(desc.rings, MIN_RINGS, MAX_RINGS);
  clamped.subdivisions = Math::clamp(desc.subdivisions, 1u, MAX_SUBDIVISIONS);
  return clamped;
}

/*
 */
ShapeDesc
MeshGenerator::getCanonical(const ShapeDesc& desc)
{
  const ShapeDesc clamped = getClamped(desc);
  const ShapeDesc defaults;
  ShapeDesc canonical{.type = clamped.type};
  switch (clamped.type) {
    case ShapeType::Plane:
      canonical.size = Vector3(clamped.size.x, clamped.size.y, defaults.size.z);
      canonical.subdivisions = clamped.subdivisions;
      break;
    case ShapeType::Box:
      canonical.size = clamped.size;
      break;
    case ShapeType::Sphere:
      canonical.radius = clamped.radius;
      canonical.segments = clamped.segments;
      canonical.rings = clamped.rings;
      break;
    case ShapeType::Capsule:
      canonical.radius = clamped.radius;
      canonical.height = clamped.height;
      canonical.segments = clamped.segments;
      canonical.rings = clamped.rings;
      break;
    case ShapeType::Cylinder:
    case ShapeType::Cone:
      canonical.radius = clamped.radius;
      canonical.height = clamped.height;
      canonical.segments = clamped.segments;
      break;
    case ShapeType::COUNT:
      break;
  }
  return canonical;
}

/*
 */
const ANSICHAR*
MeshGenerator::getName(ShapeType type)
{
  switch (type) {
    case ShapeType::Plane:
      return "Plane";
    case ShapeType::Box:
      return "Box";
    case ShapeType::Sphere:
      return "Sphere";
    case ShapeType::Capsule:
      return "Capsule";
    case ShapeType::Cylinder:
      return "Cylinder";
    case ShapeType::Cone:
      return "Cone";
    case ShapeType::COUNT:
      break;
  }
  return "Unknown";
}

} // namespace chEngineSDK
