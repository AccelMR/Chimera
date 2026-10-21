/************************************************************************/
/**
 * @file chMeshGenerator.h
 * @author AccelMR
 * @date 2026/10/18
 * @brief
 *  Builds the meshes of simple shapes from their sizes.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chVector3.h"

namespace chEngineSDK {
class Mesh;

/**
 * The shapes MeshGenerator builds. The values are saved in scenes, so they are fixed.
 */
enum class ShapeType : uint32
{
  Plane = 0,
  Box = 1,
  Sphere = 2,
  Capsule = 3,
  Cylinder = 4,
  Cone = 5,
  COUNT
};

/**
 * Sizes of a shape. Each type reads only its own fields, so a shape can change type and
 * come back with the values it had.
 */
struct ShapeDesc
{
  ShapeType type = ShapeType::Box;
  // Plane: x and y. Box: the three sides.
  Vector3 size = Vector3(1.0f, 1.0f, 1.0f);
  // Sphere, capsule, cylinder and cone.
  float radius = 0.5f;
  // Along Z. A capsule's height goes from tip to tip.
  float height = 2.0f;
  // Around the Z axis: sphere, capsule, cylinder and cone.
  uint32 segments = 32;
  // From pole to pole: sphere and capsule.
  uint32 rings = 16;
  // Quads along each side of a plane.
  uint32 subdivisions = 1;

  NODISCARD bool
  operator==(const ShapeDesc& other) const = default;

  NODISCARD CH_CORE_EXPORT uint64
  getHash() const;
};

/**
 * Exists so simple shapes need no asset files: their meshes are computed, with exact
 * normals, texture coordinates and tangents, from a ShapeDesc. Every shape is centered on
 * the origin, with Z as its axis (the plane faces up). Seen from outside, the texture is
 * never mirrored.
 */
class CH_CORE_EXPORT MeshGenerator
{
 public:
  /**
   * Replaces the vertices and indices of outMesh with the shape, made from
   * getClamped(desc). Vertices are VertexStatic; indices are 16 bits when they fit.
   */
  static void
  generate(const ShapeDesc& desc, Mesh& outMesh);

  /**
   * desc with every field inside the range the generator accepts.
   */
  NODISCARD static ShapeDesc
  getClamped(const ShapeDesc& desc);

  /**
   * getClamped(desc) with the fields its type does not read set to their defaults, so two
   * descriptions that give the same mesh compare equal.
   */
  NODISCARD static ShapeDesc
  getCanonical(const ShapeDesc& desc);

  /**
   * A null terminated literal; "Unknown" for a value outside the enum.
   */
  NODISCARD static const ANSICHAR*
  getName(ShapeType type);

  static constexpr float MIN_SIZE = 0.001f;
  static constexpr uint32 MIN_SEGMENTS = 3;
  static constexpr uint32 MAX_SEGMENTS = 256;
  static constexpr uint32 MIN_RINGS = 2;
  static constexpr uint32 MAX_RINGS = 128;
  static constexpr uint32 MAX_SUBDIVISIONS = 256;
};

} // namespace chEngineSDK
