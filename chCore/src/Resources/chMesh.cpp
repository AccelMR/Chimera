/************************************************************************/
/**
 * @file chMesh.cpp
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 * Mesh data structures and types used in the engine.
 */
/************************************************************************/

#include "chMesh.h"

#include "chIBuffer.h"
#include "chIGraphicsAPI.h"
#include "chMath.h"
#include "chRay.h"
#include "chRayCast.h"

namespace chEngineSDK {

/*
 */
VertexSkin
MeshSkin::packVertex(Span<const BoneInfluence> influences)
{
  // Strongest first, kept by insertion into a small array.
  BoneInfluence strongest[MAX_INFLUENCES] = {};
  uint32 count = 0;
  for (const BoneInfluence& influence : influences) {
    // Also skips NaN.
    if (!(influence.weight > 0.0f)) {
      continue;
    }
    uint32 position = count;
    while (position > 0 && strongest[position - 1].weight < influence.weight) {
      if (position < MAX_INFLUENCES) {
        strongest[position] = strongest[position - 1];
      }
      --position;
    }
    if (position < MAX_INFLUENCES) {
      strongest[position] = influence;
      count = Math::min(count + 1, MAX_INFLUENCES);
    }
  }

  VertexSkin vertex{.bones = {0, 0, 0, 0}, .weights = {255, 0, 0, 0}};
  float total = 0.0f;
  for (uint32 i = 0; i < count; ++i) {
    total += strongest[i].weight;
  }
  if (count == 0 || !(total > 0.0f) || !Math::isFinite(total)) {
    return vertex;
  }

  int32 sum = 0;
  int32 weights[MAX_INFLUENCES] = {};
  for (uint32 i = 0; i < count; ++i) {
    CH_ASSERT(strongest[i].bone < MAX_BONES);
    vertex.bones[i] = static_cast<uint8>(strongest[i].bone);
    weights[i] = static_cast<int32>(strongest[i].weight / total * 255.0f + 0.5f);
    sum += weights[i];
  }
  // The strongest weight is at least a quarter of 255, so the few units of rounding never
  // take it below zero.
  weights[0] += 255 - sum;
  for (uint32 i = 0; i < MAX_INFLUENCES; ++i) {
    vertex.weights[i] = static_cast<uint8>(weights[i]);
  }
  return vertex;
}

/*
 */
const SphereBoxBounds&
Mesh::getBounds() const
{
  if (!m_bBoundsDirty) {
    return m_bounds;
  }
  m_bBoundsDirty = false;
  m_bounds = SphereBoxBounds(Vector3::ZERO, Vector3::ZERO, 0.0f);

  const uint8* firstPosition = getFirstPosition();
  if (!firstPosition) {
    return m_bounds;
  }
  const uint32 stride = m_vertexLayout.getVertexSize();
  const auto readPosition = [firstPosition, stride](uint32 index) {
    return readPositionAt(firstPosition, stride, index);
  };

  // Two passes: the box gives the center, then the sphere is the smallest around that
  // center that holds every vertex, tighter than the half diagonal of the box.
  const Vector3 firstPoint = readPosition(0);
  AABox box(firstPoint, firstPoint);
  for (uint32 i = 1; i < m_vertexCount; ++i) {
    box += readPosition(i);
  }

  const Vector3 center = box.getCenter();
  float maxSquareDistance = 0.0f;
  for (uint32 i = 0; i < m_vertexCount; ++i) {
    maxSquareDistance = Math::max(maxSquareDistance, readPosition(i).sqrDistance(center));
  }

  m_bounds = SphereBoxBounds(center, box.getExtent(), Math::sqrt(maxSquareDistance));
  return m_bounds;
}

/*
 */
bool
Mesh::raycast(const Ray& localRay, float& outDistance) const
{
  const uint8* firstPosition = getFirstPosition();
  if (!firstPosition) {
    return false;
  }
  const uint32 stride = m_vertexLayout.getVertexSize();

  const bool bHasIndices = m_indexCount > 0;
  const SIZE_T indexSize = m_indexType == IndexType::UInt16 ? sizeof(uint16) : sizeof(uint32);
  const uint32 cornerCount = bHasIndices ? m_indexCount : m_vertexCount;
  if (bHasIndices && indexSize * m_indexCount > m_indexData.size()) {
    return false;
  }
  const auto readIndex = [this, bHasIndices, indexSize](uint32 corner) -> uint32 {
    if (!bHasIndices) {
      return corner;
    }
    const uint8* source = m_indexData.data() + corner * indexSize;
    if (indexSize == sizeof(uint16)) {
      uint16 index;
      std::memcpy(&index, source, sizeof(index));
      return index;
    }
    uint32 index;
    std::memcpy(&index, source, sizeof(index));
    return index;
  };

  bool bHit = false;
  for (uint32 corner = 0; corner + 2 < cornerCount; corner += 3) {
    const uint32 a = readIndex(corner);
    const uint32 b = readIndex(corner + 1);
    const uint32 c = readIndex(corner + 2);
    if (a >= m_vertexCount || b >= m_vertexCount || c >= m_vertexCount) {
      continue;
    }

    float distance = 0.0f;
    if (RayCast::triangle(localRay, readPositionAt(firstPosition, stride, a),
                          readPositionAt(firstPosition, stride, b),
                          readPositionAt(firstPosition, stride, c), distance) &&
        (!bHit || distance < outDistance)) {
      outDistance = distance;
      bHit = true;
    }
  }
  return bHit;
}

/*
 */
bool
Mesh::readPositions(Vector<Vector3>& outPositions) const
{
  outPositions.clear();
  const uint8* firstPosition = getFirstPosition();
  if (!firstPosition) {
    return false;
  }
  const uint32 stride = m_vertexLayout.getVertexSize();
  outPositions.resize(m_vertexCount);
  for (uint32 i = 0; i < m_vertexCount; ++i) {
    outPositions[i] = readPositionAt(firstPosition, stride, i);
  }
  return true;
}

/*
 */
bool
Mesh::readIndices(Vector<uint32>& outIndices) const
{
  outIndices.clear();
  if (m_indexCount == 0) {
    outIndices.resize(m_vertexCount);
    for (uint32 i = 0; i < m_vertexCount; ++i) {
      outIndices[i] = i;
    }
    return true;
  }

  const bool bShort = m_indexType == IndexType::UInt16;
  const SIZE_T indexSize = bShort ? sizeof(uint16) : sizeof(uint32);
  if (indexSize * m_indexCount > m_indexData.size()) {
    return false;
  }
  outIndices.resize(m_indexCount);
  if (!bShort) {
    std::memcpy(outIndices.data(), m_indexData.data(), indexSize * m_indexCount);
    return true;
  }
  for (uint32 i = 0; i < m_indexCount; ++i) {
    uint16 index;
    std::memcpy(&index, m_indexData.data() + i * indexSize, sizeof(index));
    outIndices[i] = index;
  }
  return true;
}

/*
 */
const uint8*
Mesh::getFirstPosition() const
{
  const VertexAttributeDesc* position = nullptr;
  for (const VertexAttributeDesc& attribute : m_vertexLayout.getAttributes()) {
    if (attribute.type == VertexAttributeType::Position) {
      position = &attribute;
      break;
    }
  }
  const uint32 stride = m_vertexLayout.getVertexSize();
  if (!position || position->format != VertexFormat::Float3 || m_vertexCount == 0 ||
      static_cast<SIZE_T>(stride) * m_vertexCount > m_vertexData.size()) {
    return nullptr;
  }
  return m_vertexData.data() + position->offset;
}

/*
 */
Vector3
Mesh::readPositionAt(const uint8* firstPosition, uint32 stride, uint32 index)
{
  // The vertex data is a byte array, so a position may not be aligned for a float read.
  float xyz[3];
  std::memcpy(xyz, firstPosition + static_cast<SIZE_T>(index) * stride, sizeof(xyz));
  return Vector3(xyz[0], xyz[1], xyz[2]);
}

/*
 */
const IBuffer*
Mesh::getVertexBuffer() const
{
  if (!m_vertexBuffer) {
    createGpuBuffers();
  }
  return m_vertexBuffer.get();
}

/*
 */
const IBuffer*
Mesh::getIndexBuffer() const
{
  if (!m_indexBuffer) {
    createGpuBuffers();
  }
  return m_indexBuffer.get();
}

/*
 */
void
Mesh::createGpuBuffers() const
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();

  if (!m_vertexBuffer && !m_vertexData.empty()) {
    const uint32 size = static_cast<uint32>(m_vertexData.size());
    m_vertexBuffer = graphicsAPI.createBuffer({.size = size,
                                               .usage = BufferUsage::VertexBuffer,
                                               .memoryUsage = MemoryUsage::GpuOnly,
                                               .initialData = m_vertexData.data(),
                                               .initialDataSize = size});
  }

  if (!m_indexBuffer && !m_indexData.empty()) {
    const uint32 size = static_cast<uint32>(m_indexData.size());
    m_indexBuffer = graphicsAPI.createBuffer({.size = size,
                                              .usage = BufferUsage::IndexBuffer,
                                              .memoryUsage = MemoryUsage::GpuOnly,
                                              .initialData = m_indexData.data(),
                                              .initialDataSize = size});
  }
}

} // namespace chEngineSDK
