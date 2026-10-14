/************************************************************************/
/**
 * @file chMeshSplit.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Cuts a mesh into the pieces that do not touch.
 */
/************************************************************************/
#include "chMeshSplit.h"

#include <limits>

#include "chAlgorithm.h"
#include "chBox.h"
#include "chMath.h"
#include "chMesh.h"
#include "chShapeOverlap.h"

namespace chEngineSDK {
namespace {
constexpr uint32 kNone = std::numeric_limits<uint32>::max();

/*
 * Every element starts as its own set.
 */
Vector<uint32>
makeSets(SIZE_T count)
{
  Vector<uint32> parents(count);
  for (SIZE_T i = 0; i < count; ++i) {
    parents[i] = static_cast<uint32>(i);
  }
  return parents;
}

/*
 */
uint32
findRoot(Vector<uint32>& parents, uint32 element)
{
  while (parents[element] != element) {
    // Halving the path keeps later searches short.
    parents[element] = parents[parents[element]];
    element = parents[element];
  }
  return element;
}

/*
 */
void
joinSets(Vector<uint32>& parents, uint32 a, uint32 b)
{
  const uint32 rootA = findRoot(parents, a);
  const uint32 rootB = findRoot(parents, b);
  if (rootA != rootB) {
    parents[Math::max(rootA, rootB)] = Math::min(rootA, rootB);
  }
}

/*
 */
bool
isPositionBefore(const Vector3& a, const Vector3& b) noexcept
{
  if (a.x != b.x) {
    return a.x < b.x;
  }
  if (a.y != b.y) {
    return a.y < b.y;
  }
  return a.z < b.z;
}

/*
 * Joins the pieces whose boxes overlap, again and again, because a joined box is bigger and
 * may reach pieces it did not reach before. Each piece's box ends up in its root.
 */
void
joinOverlappingPieces(Vector<uint32>& parents, Vector<AABox>& boxes)
{
  Vector<uint32> roots;
  bool bJoined = true;
  while (bJoined) {
    bJoined = false;
    roots.clear();
    for (uint32 i = 0; i < static_cast<uint32>(parents.size()); ++i) {
      if (parents[i] == i) {
        roots.push_back(i);
      }
    }
    // Sorted by the start of the box on X, so each piece is only tested against those that
    // start before it ends.
    Algorithm::sort(roots, [&boxes](uint32 a, uint32 b) {
      return boxes[a].minPoint.x < boxes[b].minPoint.x;
    });

    for (SIZE_T a = 0; a < roots.size(); ++a) {
      const uint32 rootA = roots[a];
      if (parents[rootA] != rootA) {
        continue;
      }
      for (SIZE_T b = a + 1;
           b < roots.size() && boxes[roots[b]].minPoint.x <= boxes[rootA].maxPoint.x; ++b) {
        const uint32 rootB = roots[b];
        if (parents[rootB] == rootB && ShapeOverlap::boxBox(boxes[rootA], boxes[rootB])) {
          parents[rootB] = rootA;
          boxes[rootA] += boxes[rootB];
          bJoined = true;
        }
      }
    }
  }
}

/*
 */
SPtr<Mesh>
makePartMesh(const Mesh& source, Vector<uint8> vertexData, uint32 vertexCount,
             const Vector<uint32>& indices)
{
  SPtr<Mesh> part = chMakeShared<Mesh>();
  part->setVertexData(vertexData, vertexCount);
  part->setVertexLayout(source.getVertexLayout());
  part->setMaterialSlot(source.getMaterialSlot());
  if (vertexCount > std::numeric_limits<uint16>::max()) {
    part->setIndexData(indices);
    return part;
  }
  Vector<uint16> shortIndices(indices.size());
  for (SIZE_T i = 0; i < indices.size(); ++i) {
    shortIndices[i] = static_cast<uint16>(indices[i]);
  }
  part->setIndexData(shortIndices);
  return part;
}
} // namespace

/*
 */
Vector<SPtr<Mesh>>
MeshSplit::splitParts(const SPtr<Mesh>& mesh)
{
  Vector<SPtr<Mesh>> parts;
  if (!mesh) {
    return parts;
  }
  parts.push_back(mesh);

  Vector<Vector3> positions;
  Vector<uint32> indices;
  if (!mesh->readPositions(positions) || !mesh->readIndices(indices) ||
      indices.size() < 6) {
    return parts;
  }
  const uint32 vertexCount = static_cast<uint32>(positions.size());
  const uint32 triangleCount = static_cast<uint32>(indices.size() / 3);
  for (const uint32 index : indices) {
    if (index >= vertexCount) {
      return parts;
    }
  }
  // A NaN would break the sort below.
  for (const Vector3& position : positions) {
    if (!Math::isFinite(position.x) || !Math::isFinite(position.y) ||
        !Math::isFinite(position.z)) {
      return parts;
    }
  }

  // Vertices at the same position, then the corners of each triangle.
  Vector<uint32> vertexSets = makeSets(vertexCount);
  Vector<uint32> byPosition = makeSets(vertexCount);
  Algorithm::sort(byPosition, [&positions](uint32 a, uint32 b) {
    return isPositionBefore(positions[a], positions[b]);
  });
  for (uint32 i = 1; i < vertexCount; ++i) {
    if (positions[byPosition[i]] == positions[byPosition[i - 1]]) {
      joinSets(vertexSets, byPosition[i], byPosition[i - 1]);
    }
  }
  for (uint32 t = 0; t < triangleCount; ++t) {
    joinSets(vertexSets, indices[t * 3], indices[t * 3 + 1]);
    joinSets(vertexSets, indices[t * 3 + 1], indices[t * 3 + 2]);
  }

  // One piece per set of vertices that triangles use, numbered as the triangles meet them.
  Vector<uint32> pieceOfSet(vertexCount, kNone);
  Vector<uint32> pieceOfTriangle(triangleCount);
  Vector<AABox> boxes;
  for (uint32 t = 0; t < triangleCount; ++t) {
    const uint32 set = findRoot(vertexSets, indices[t * 3]);
    if (pieceOfSet[set] == kNone) {
      pieceOfSet[set] = static_cast<uint32>(boxes.size());
      const Vector3& first = positions[indices[t * 3]];
      boxes.emplace_back(first, first);
    }
    const uint32 piece = pieceOfSet[set];
    pieceOfTriangle[t] = piece;
    for (uint32 corner = 0; corner < 3; ++corner) {
      boxes[piece] += positions[indices[t * 3 + corner]];
    }
  }
  if (boxes.size() < 2) {
    return parts;
  }

  Vector<uint32> pieceSets = makeSets(boxes.size());
  joinOverlappingPieces(pieceSets, boxes);

  // Final parts, numbered as the triangles meet them. A vertex belongs to one piece, so one
  // table numbers it inside its part.
  Vector<uint32> partOfRoot(boxes.size(), kNone);
  Vector<uint32> partOfTriangle(triangleCount);
  uint32 partCount = 0;
  for (uint32 t = 0; t < triangleCount; ++t) {
    const uint32 root = findRoot(pieceSets, pieceOfTriangle[t]);
    if (partOfRoot[root] == kNone) {
      partOfRoot[root] = partCount++;
    }
    partOfTriangle[t] = partOfRoot[root];
  }
  if (partCount < 2) {
    return parts;
  }

  const uint32 stride = mesh->getVertexLayout().getVertexSize();
  const Vector<uint8>& sourceVertices = mesh->getVertexData();
  Vector<Vector<uint8>> partVertices(partCount);
  Vector<Vector<uint32>> partIndices(partCount);
  Vector<uint32> partVertexCounts(partCount, 0);
  Vector<uint32> newIndex(vertexCount, kNone);
  for (uint32 t = 0; t < triangleCount; ++t) {
    const uint32 part = partOfTriangle[t];
    for (uint32 corner = 0; corner < 3; ++corner) {
      const uint32 vertex = indices[t * 3 + corner];
      if (newIndex[vertex] == kNone) {
        newIndex[vertex] = partVertexCounts[part]++;
        const uint8* bytes = sourceVertices.data() + static_cast<SIZE_T>(vertex) * stride;
        partVertices[part].insert(partVertices[part].end(), bytes, bytes + stride);
      }
      partIndices[part].push_back(newIndex[vertex]);
    }
  }

  parts.clear();
  parts.reserve(partCount);
  for (uint32 part = 0; part < partCount; ++part) {
    parts.push_back(makePartMesh(*mesh, std::move(partVertices[part]),
                                 partVertexCounts[part], partIndices[part]));
  }
  return parts;
}

} // namespace chEngineSDK
