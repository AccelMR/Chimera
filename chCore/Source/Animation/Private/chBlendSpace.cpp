/************************************************************************/
/**
 * @file chBlendSpace.cpp
 * @author AccelMR
 * @date 2026/10/23
 * @brief
 *  Turns the parameters of a blend space into the weights of its clips.
 */
/************************************************************************/
#include "chBlendSpace.h"

#include "chAlgorithm.h"
#include "chAnimationGraph.h"
#include "chMath.h"

namespace chEngineSDK {
namespace {
// A point this far outside a triangle still counts as inside, so points on a shared edge
// never fall between two triangles.
constexpr float kInsideTolerance = 1e-4f;
// Below this, three points count as on one line.
constexpr double kDegenerateTolerance = 1e-12;

struct Point
{
  double x;
  double y;
};

struct Triangle
{
  uint32 a;
  uint32 b;
  uint32 c;
  // Circumscribed circle: center and squared radius.
  double centerX;
  double centerY;
  double radiusSquared;
};

NODISCARD bool
makeTriangle(const Vector<Point>& points, uint32 a, uint32 b, uint32 c, Triangle& out)
{
  const Point& pa = points[a];
  const Point& pb = points[b];
  const Point& pc = points[c];
  const double d = 2.0 * (pa.x * (pb.y - pc.y) + pb.x * (pc.y - pa.y) + pc.x * (pa.y - pb.y));
  if (d > -kDegenerateTolerance && d < kDegenerateTolerance) {
    return false;
  }
  const double a2 = pa.x * pa.x + pa.y * pa.y;
  const double b2 = pb.x * pb.x + pb.y * pb.y;
  const double c2 = pc.x * pc.x + pc.y * pc.y;
  out.a = a;
  out.b = b;
  out.c = c;
  out.centerX = (a2 * (pb.y - pc.y) + b2 * (pc.y - pa.y) + c2 * (pa.y - pb.y)) / d;
  out.centerY = (a2 * (pc.x - pb.x) + b2 * (pa.x - pc.x) + c2 * (pb.x - pa.x)) / d;
  const double dx = pa.x - out.centerX;
  const double dy = pa.y - out.centerY;
  out.radiusSquared = dx * dx + dy * dy;
  return true;
}

/*
 * Adds the edge, or removes it when it is already there: what is left after every edge of
 * a group of triangles is the outline of the group.
 */
void
toggleEdge(Vector<Pair<uint32, uint32>>& edges, uint32 a, uint32 b)
{
  const Pair<uint32, uint32> edge =
      a < b ? Pair<uint32, uint32>(a, b) : Pair<uint32, uint32>(b, a);
  for (SIZE_T i = 0; i < edges.size(); ++i) {
    if (edges[i] == edge) {
      edges[i] = edges.back();
      edges.pop_back();
      return;
    }
  }
  edges.push_back(edge);
}

/*
 * Bowyer-Watson: each point removes the triangles whose circle holds it and fills the
 * hole with triangles to it. Starts from a triangle around the unit box whose corners are
 * the indices past the points, dropped at the end.
 */
NODISCARD Vector<Triangle>
triangulate(Vector<Point> points)
{
  const uint32 count = static_cast<uint32>(points.size());
  points.push_back({-30.0, -10.0});
  points.push_back({31.0, -10.0});
  points.push_back({0.5, 30.0});

  Vector<Triangle> triangles;
  Triangle super;
  if (makeTriangle(points, count, count + 1, count + 2, super)) {
    triangles.push_back(super);
  }

  Vector<Pair<uint32, uint32>> hole;
  for (uint32 i = 0; i < count; ++i) {
    const Point& point = points[i];
    hole.clear();
    for (SIZE_T t = 0; t < triangles.size();) {
      const Triangle& triangle = triangles[t];
      const double dx = point.x - triangle.centerX;
      const double dy = point.y - triangle.centerY;
      if (dx * dx + dy * dy < triangle.radiusSquared * (1.0 - 1e-9)) {
        toggleEdge(hole, triangle.a, triangle.b);
        toggleEdge(hole, triangle.b, triangle.c);
        toggleEdge(hole, triangle.c, triangle.a);
        triangles[t] = triangles.back();
        triangles.pop_back();
      }
      else {
        ++t;
      }
    }
    for (const Pair<uint32, uint32>& edge : hole) {
      Triangle triangle;
      if (makeTriangle(points, edge.first, edge.second, i, triangle)) {
        triangles.push_back(triangle);
      }
    }
  }

  Algorithm::removeAllIf(triangles, [count](const Triangle& triangle) {
    return triangle.a >= count || triangle.b >= count || triangle.c >= count;
  });
  return triangles;
}
} // namespace

/*
 */
void
BlendSpace::build(const AnimationState& state)
{
  const Vector<BlendSpaceSample>& samples = state.samples;
  const uint32 count = static_cast<uint32>(samples.size());
  m_b2D = state.kind == AnimationStateKind::BlendSpace2D;
  m_points.clear();
  m_triangles.clear();
  m_outline.clear();
  m_lineOrder.resize(count);
  m_lineKeys.resize(count);

  if (!m_b2D) {
    for (uint32 i = 0; i < count; ++i) {
      m_lineOrder[i] = i;
    }
    Algorithm::sort(m_lineOrder,
                    [&samples](uint32 a, uint32 b) { return samples[a].x < samples[b].x; });
    for (uint32 i = 0; i < count; ++i) {
      m_lineKeys[i] = samples[m_lineOrder[i]].x;
    }
    return;
  }

  float maxX = count > 0 ? samples[0].x : 0.0f;
  float maxY = count > 0 ? samples[0].y : 0.0f;
  m_minX = maxX;
  m_minY = maxY;
  for (const BlendSpaceSample& sample : samples) {
    m_minX = Math::min(m_minX, sample.x);
    m_minY = Math::min(m_minY, sample.y);
    maxX = Math::max(maxX, sample.x);
    maxY = Math::max(maxY, sample.y);
  }
  m_invRangeX = maxX - m_minX > 1e-6f ? 1.0f / (maxX - m_minX) : 1.0f;
  m_invRangeY = maxY - m_minY > 1e-6f ? 1.0f / (maxY - m_minY) : 1.0f;

  Vector<Point> points(count);
  m_points.resize(count * 2);
  for (uint32 i = 0; i < count; ++i) {
    m_points[i * 2] = (samples[i].x - m_minX) * m_invRangeX;
    m_points[i * 2 + 1] = (samples[i].y - m_minY) * m_invRangeY;
    points[i] = {m_points[i * 2], m_points[i * 2 + 1]};
  }

  Vector<Pair<uint32, uint32>> outline;
  for (const Triangle& triangle : triangulate(points)) {
    m_triangles.insert(m_triangles.end(), {triangle.a, triangle.b, triangle.c});
    toggleEdge(outline, triangle.a, triangle.b);
    toggleEdge(outline, triangle.b, triangle.c);
    toggleEdge(outline, triangle.c, triangle.a);
  }
  for (const Pair<uint32, uint32>& edge : outline) {
    m_outline.insert(m_outline.end(), {edge.first, edge.second});
  }
  if (!m_triangles.empty()) {
    return;
  }

  // No triangle: every sample is on one line, from the two that are farthest apart.
  uint32 first = 0;
  uint32 last = 0;
  float farthest = -1.0f;
  for (uint32 i = 0; i < count; ++i) {
    for (uint32 j = i + 1; j < count; ++j) {
      const float dx = m_points[j * 2] - m_points[i * 2];
      const float dy = m_points[j * 2 + 1] - m_points[i * 2 + 1];
      if (dx * dx + dy * dy > farthest) {
        farthest = dx * dx + dy * dy;
        first = i;
        last = j;
      }
    }
  }
  if (count > 0) {
    m_lineOriginX = m_points[first * 2];
    m_lineOriginY = m_points[first * 2 + 1];
  }
  if (farthest > 0.0f) {
    const float length = Math::sqrt(farthest);
    m_lineDirectionX = (m_points[last * 2] - m_lineOriginX) / length;
    m_lineDirectionY = (m_points[last * 2 + 1] - m_lineOriginY) / length;
  }
  for (uint32 i = 0; i < count; ++i) {
    m_lineOrder[i] = i;
  }
  const auto keyOf = [this](uint32 i) {
    return (m_points[i * 2] - m_lineOriginX) * m_lineDirectionX
           + (m_points[i * 2 + 1] - m_lineOriginY) * m_lineDirectionY;
  };
  Algorithm::sort(m_lineOrder, [&keyOf](uint32 a, uint32 b) { return keyOf(a) < keyOf(b); });
  for (uint32 i = 0; i < count; ++i) {
    m_lineKeys[i] = keyOf(m_lineOrder[i]);
  }
}

/*
 */
BlendSpaceWeights
BlendSpace::computeLineWeights(float key) const noexcept
{
  BlendSpaceWeights result;
  const SIZE_T count = m_lineOrder.size();
  if (count == 0) {
    return result;
  }
  result.count = 1;
  result.items[0].weight = 1.0f;
  if (key <= m_lineKeys[0]) {
    result.items[0].sample = m_lineOrder[0];
    return result;
  }
  if (key >= m_lineKeys[count - 1]) {
    result.items[0].sample = m_lineOrder[count - 1];
    return result;
  }
  SIZE_T i = 0;
  while (key >= m_lineKeys[i + 1]) {
    ++i;
  }
  const float alpha = (key - m_lineKeys[i]) / (m_lineKeys[i + 1] - m_lineKeys[i]);
  result.items[0] = {.sample = m_lineOrder[i], .weight = 1.0f - alpha};
  if (alpha > 0.0f) {
    result.items[1] = {.sample = m_lineOrder[i + 1], .weight = alpha};
    result.count = 2;
  }
  return result;
}

/*
 */
BlendSpaceWeights
BlendSpace::computeWeights(float x, float y) const noexcept
{
  if (!m_b2D) {
    return computeLineWeights(x);
  }
  const float qx = (x - m_minX) * m_invRangeX;
  const float qy = (y - m_minY) * m_invRangeY;
  if (m_triangles.empty()) {
    return computeLineWeights((qx - m_lineOriginX) * m_lineDirectionX
                              + (qy - m_lineOriginY) * m_lineDirectionY);
  }

  BlendSpaceWeights result;
  for (SIZE_T t = 0; t < m_triangles.size(); t += 3) {
    const uint32 a = m_triangles[t];
    const uint32 b = m_triangles[t + 1];
    const uint32 c = m_triangles[t + 2];
    const float ax = m_points[a * 2];
    const float ay = m_points[a * 2 + 1];
    const float abx = m_points[b * 2] - ax;
    const float aby = m_points[b * 2 + 1] - ay;
    const float acx = m_points[c * 2] - ax;
    const float acy = m_points[c * 2 + 1] - ay;
    const float aqx = qx - ax;
    const float aqy = qy - ay;
    const float area = abx * acy - aby * acx;
    const float wb = (aqx * acy - aqy * acx) / area;
    const float wc = (abx * aqy - aby * aqx) / area;
    const float wa = 1.0f - wb - wc;
    if (wa < -kInsideTolerance || wb < -kInsideTolerance || wc < -kInsideTolerance) {
      continue;
    }
    const float clampedA = Math::max(wa, 0.0f);
    const float clampedB = Math::max(wb, 0.0f);
    const float clampedC = Math::max(wc, 0.0f);
    const float sum = clampedA + clampedB + clampedC;
    result.items[0] = {.sample = a, .weight = clampedA / sum};
    result.items[1] = {.sample = b, .weight = clampedB / sum};
    result.items[2] = {.sample = c, .weight = clampedC / sum};
    result.count = 3;
    return result;
  }

  // Outside every triangle: the closest point of the outline.
  float closest = Math::MAX_FLOAT;
  for (SIZE_T e = 0; e < m_outline.size(); e += 2) {
    const uint32 a = m_outline[e];
    const uint32 b = m_outline[e + 1];
    const float ax = m_points[a * 2];
    const float ay = m_points[a * 2 + 1];
    const float abx = m_points[b * 2] - ax;
    const float aby = m_points[b * 2 + 1] - ay;
    const float alpha =
        Math::clamp(((qx - ax) * abx + (qy - ay) * aby) / (abx * abx + aby * aby), 0.0f, 1.0f);
    const float dx = ax + abx * alpha - qx;
    const float dy = ay + aby * alpha - qy;
    const float distance = dx * dx + dy * dy;
    if (distance < closest) {
      closest = distance;
      result.items[0] = {.sample = a, .weight = 1.0f - alpha};
      result.items[1] = {.sample = b, .weight = alpha};
      result.count = 2;
    }
  }
  return result;
}

} // namespace chEngineSDK
