/************************************************************************/
/**
 * @file chBlendSpace.h
 * @author AccelMR
 * @date 2026/10/23
 * @brief
 *  Turns the parameters of a blend space into the weights of its clips.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
struct AnimationState;

struct BlendSpaceWeight
{
  // Index into the state's samples.
  uint32 sample = 0;
  float weight = 0.0f;
};

/**
 * At most three clips are blended at once: two in 1D, the corners of a triangle in 2D.
 * Weights add up to 1.
 */
struct BlendSpaceWeights
{
  static constexpr uint32 MAX_WEIGHTS = 3;

  BlendSpaceWeight items[MAX_WEIGHTS];
  uint32 count = 0;
};

/**
 * Exists so a blend space does its slow work once: build sorts the samples of a 1D space
 * and triangulates a 2D one (Delaunay, positions scaled to a unit box so axes with
 * different ranges weigh the same), and computeWeights then only walks the segments or the
 * triangles. A point outside a 2D space takes the closest point of its outline; samples on
 * one line blend as a 1D space along that line.
 */
class CH_CORE_EXPORT BlendSpace
{
 public:
  void
  build(const AnimationState& state);

  NODISCARD BlendSpaceWeights
  computeWeights(float x, float y) const noexcept;

  /**
   * Corners of each triangle as sample indices; empty in 1D and on one line.
   */
  NODISCARD FORCEINLINE const Vector<uint32>&
  getTriangles() const noexcept
  {
    return m_triangles;
  }

 private:
  NODISCARD BlendSpaceWeights
  computeLineWeights(float key) const noexcept;

  // Samples sorted along the line (the X axis in 1D) and their place on it.
  Vector<uint32> m_lineOrder;
  Vector<float> m_lineKeys;
  // 2D: the line the samples lie on when they make no triangle, as origin plus direction
  // in unit box space.
  float m_lineOriginX = 0.0f;
  float m_lineOriginY = 0.0f;
  float m_lineDirectionX = 1.0f;
  float m_lineDirectionY = 0.0f;

  bool m_b2D = false;
  float m_minX = 0.0f;
  float m_minY = 0.0f;
  float m_invRangeX = 1.0f;
  float m_invRangeY = 1.0f;
  // Sample positions in unit box space, x then y.
  Vector<float> m_points;
  // Three sample indices per triangle.
  Vector<uint32> m_triangles;
  // Two sample indices per edge of the outline.
  Vector<uint32> m_outline;
};

} // namespace chEngineSDK
