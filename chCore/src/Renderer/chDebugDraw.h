/************************************************************************/
/**
 * @file chDebugDraw.h
 * @author AccelMR
 * @date 2026/10/18
 * @brief
 *  Lines drawn over the scene to show shapes, bounds and directions.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chVector3.h"

namespace chEngineSDK {

/**
 * Whether a debug line hides behind the scene or always shows.
 */
enum class DebugDepth : uint32
{
  Tested,
  OnTop,
  COUNT
};

/**
 * One end of a debug line, as the line shader reads it.
 */
struct DebugLineVertex
{
  Vector3 position;
  // RGBA, 8 bits each, red in the lowest byte; linear values, encoded by the shader.
  uint32 color;
};
static_assert(sizeof(DebugLineVertex) == 16, "DebugLineVertex must match debuglines.hlsl");

/**
 * Exists so gameplay and editor code can show shapes, bounds and directions without a mesh
 * or a pass of their own: it collects lines in world space, and the SceneRenderer draws
 * them over the image. Each Scene has one, cleared when the scene updates, and an
 * application can hand the renderer one more. The lines are kept in vectors that keep their
 * memory after clear(), so drawing the same lines every frame does not allocate.
 * Main thread only.
 *
 * Built without CH_DEBUG_DRAW (CMake option CH_ENABLE_DEBUG_DRAW off, for shipping) every
 * function is an empty inline and the class holds nothing, so callers need no checks of
 * their own. Wrap only work done just for the lines (an expensive loop) in
 * #if USING(CH_DEBUG_DRAW).
 */
class CH_CORE_EXPORT DebugDraw
{
 public:
#if USING(CH_DEBUG_DRAW)
  void
  line(const Vector3& start,
       const Vector3& end,
       const LinearColor& color,
       DebugDepth depth = DebugDepth::Tested);

  /**
   * From the origin along the direction for length.
   */
  void
  ray(const Ray& ray,
      float length,
      const LinearColor& color,
      DebugDepth depth = DebugDepth::Tested);

  void
  box(const AABox& box, const LinearColor& color, DebugDepth depth = DebugDepth::Tested);

  /**
   * localBox moved by transform, which may rotate, scale or skew it.
   */
  void
  box(const AABox& localBox,
      const Matrix4& transform,
      const LinearColor& color,
      DebugDepth depth = DebugDepth::Tested);

  void
  orientedBox(const OrientedBox& box,
              const LinearColor& color,
              DebugDepth depth = DebugDepth::Tested);

  /**
   * The circle of radius around center in the plane facing normal.
   */
  void
  circle(const Vector3& center,
         const Vector3& normal,
         float radius,
         const LinearColor& color,
         DebugDepth depth = DebugDepth::Tested,
         uint32 segments = DEFAULT_SEGMENTS);

  /**
   * Three circles, one around each world axis.
   */
  void
  sphere(const Sphere& sphere,
         const LinearColor& color,
         DebugDepth depth = DebugDepth::Tested,
         uint32 segments = DEFAULT_SEGMENTS);

  /**
   * A circle at each end, the four side lines and the half circles over both ends.
   */
  void
  capsule(const Capsule& capsule,
          const LinearColor& color,
          DebugDepth depth = DebugDepth::Tested,
          uint32 segments = DEFAULT_SEGMENTS);

  /**
   * The eight corners of what a camera with this view * projection sees, joined.
   */
  void
  frustum(const Matrix4& viewProjection,
          const LinearColor& color,
          DebugDepth depth = DebugDepth::Tested);

  /**
   * The X, Y and Z axes of transform from its origin, red, green and blue, size long.
   */
  void
  axes(const Matrix4& transform, float size, DebugDepth depth = DebugDepth::Tested);

  /**
   * Forgets every line but keeps the memory.
   */
  void
  clear() noexcept;

  /**
   * Two vertices per line.
   */
  NODISCARD FORCEINLINE Span<const DebugLineVertex>
  getVertices(DebugDepth depth) const noexcept
  {
    return m_vertices[static_cast<uint32>(depth)];
  }

  NODISCARD FORCEINLINE uint32
  getLineCount() const noexcept
  {
    return static_cast<uint32>((m_vertices[0].size() + m_vertices[1].size()) / 2);
  }
#else
  FORCEINLINE void
  line(const Vector3&, const Vector3&, const LinearColor&, DebugDepth = DebugDepth::Tested)
  {}

  FORCEINLINE void
  ray(const Ray&, float, const LinearColor&, DebugDepth = DebugDepth::Tested)
  {}

  FORCEINLINE void
  box(const AABox&, const LinearColor&, DebugDepth = DebugDepth::Tested)
  {}

  FORCEINLINE void
  box(const AABox&, const Matrix4&, const LinearColor&, DebugDepth = DebugDepth::Tested)
  {}

  FORCEINLINE void
  orientedBox(const OrientedBox&, const LinearColor&, DebugDepth = DebugDepth::Tested)
  {}

  FORCEINLINE void
  circle(const Vector3&,
         const Vector3&,
         float,
         const LinearColor&,
         DebugDepth = DebugDepth::Tested,
         uint32 = DEFAULT_SEGMENTS)
  {}

  FORCEINLINE void
  sphere(const Sphere&,
         const LinearColor&,
         DebugDepth = DebugDepth::Tested,
         uint32 = DEFAULT_SEGMENTS)
  {}

  FORCEINLINE void
  capsule(const Capsule&,
          const LinearColor&,
          DebugDepth = DebugDepth::Tested,
          uint32 = DEFAULT_SEGMENTS)
  {}

  FORCEINLINE void
  frustum(const Matrix4&, const LinearColor&, DebugDepth = DebugDepth::Tested)
  {}

  FORCEINLINE void
  axes(const Matrix4&, float, DebugDepth = DebugDepth::Tested)
  {}

  FORCEINLINE void
  clear() noexcept
  {}

  NODISCARD FORCEINLINE Span<const DebugLineVertex>
  getVertices(DebugDepth) const noexcept
  {
    return {};
  }

  NODISCARD FORCEINLINE uint32
  getLineCount() const noexcept
  {
    return 0;
  }
#endif // USING(CH_DEBUG_DRAW)

  static constexpr uint32 DEFAULT_SEGMENTS = 32;

#if USING(CH_DEBUG_DRAW)
 private:
  /**
   * An arc of radius around center from angle start to end (radians), starting along axisA
   * and turning towards axisB.
   */
  void
  arc(const Vector3& center,
      const Vector3& axisA,
      const Vector3& axisB,
      float radius,
      float startAngle,
      float endAngle,
      uint32 segments,
      uint32 color,
      DebugDepth depth);

  void
  addLine(const Vector3& start, const Vector3& end, uint32 color, DebugDepth depth);

  /**
   * The twelve edges of a box from its corners, numbered by bits (x, y, z) of max.
   */
  void
  addBoxEdges(const Vector3 (&corners)[8], uint32 color, DebugDepth depth);

  Vector<DebugLineVertex> m_vertices[static_cast<uint32>(DebugDepth::COUNT)];
  bool m_bWarnedFull = false;
#endif // USING(CH_DEBUG_DRAW)
};

} // namespace chEngineSDK
