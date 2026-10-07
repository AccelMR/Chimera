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

namespace chEngineSDK {

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
    return m_bounds;
  }

  // The vertex data is a byte array, so a position may not be aligned for a float read.
  const uint8* firstPosition = m_vertexData.data() + position->offset;
  const auto readPosition = [firstPosition, stride](uint32 index) {
    float xyz[3];
    std::memcpy(xyz, firstPosition + static_cast<SIZE_T>(index) * stride, sizeof(xyz));
    return Vector3(xyz[0], xyz[1], xyz[2]);
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
