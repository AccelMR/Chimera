/************************************************************************/
/**
 * @file chMesh.h
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 * Mesh data structures and types used in the engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include <cstring>

#include "chGraphicsTypes.h"
#include "chSphereBoxBounds.h"
#include "chVertexLayout.h"

namespace chEngineSDK {
/**
 * Class to store mesh data.
 * This is a simple data structure without much functionality.
 * It just stores the data needed for rendering a mesh.
 */
class CH_CORE_EXPORT Mesh
{
 public:
  Mesh() = default;
  ~Mesh() = default;

  /**
  * Set vertex data using a templated vertex type
  *
  * @tparam T Vertex type (e.g., VertexPosColor, VertexNormalTexCoord)
  * @param vertices Vector of vertex data
  */
  template<typename T>
  void
  setVertexData(const Vector<T>& vertices) {
    m_vertexCount = static_cast<uint32>(vertices.size());
    SIZE_T size = sizeof(T) * vertices.size();
    m_vertexData.resize(size);
    if (!vertices.empty()) {
      std::memcpy(m_vertexData.data(), vertices.data(), size);
    }
    m_vertexLayout = T::getLayout();
    onDataChanged();
  }

  /**
   * Set index data using a templated index type
   *
   * @tparam T Index type (uint16 or uint32)
   * @param indices Vector of index data
  */
  template<typename T>
  void setIndexData(const Vector<T>& indices) {
    static_assert(std::is_same<T, uint16>::value || std::is_same<T, uint32>::value,
                 "Index type must be uint16 or uint32");

    m_indexCount = static_cast<uint32>(indices.size());
    m_indexType = std::is_same<T, uint16>::value ? IndexType::UInt16 : IndexType::UInt32;
    SIZE_T size = sizeof(T) * indices.size();
    m_indexData.resize(size);
    if (!indices.empty()) {
      memcpy(m_indexData.data(), indices.data(), size);
    }
    onDataChanged();
  }

  /**
   * Set the vertex layout for the mesh
   *
   * @param layout Vertex layout
  */
  FORCEINLINE void
  setVertexLayout(const VertexLayout& layout)
  {
    m_vertexLayout = layout;
    onDataChanged();
  }

  /**
   * Get raw vertex data
   *
   * @return Pointer to vertex data
  */
  NODISCARD FORCEINLINE const Vector<uint8>&
  getVertexData() const { return m_vertexData; }

  FORCEINLINE void
  setVertexData(const Vector<uint8>& data, uint32 vertexCount) {
    m_vertexData = data;
    m_vertexCount = vertexCount;
    onDataChanged();
  }

  /**
   * Access vertex data as specified type
   *
   * @tparam T Vertex type (e.g., VertexPosColor, VertexNormalTexCoord)
   * @return Vector of vertex data
  */
  template<typename T>
  NODISCARD FORCEINLINE const Vector<T>
  getVertexData() const {
    Vector<T> vertices;

    if (m_vertexData.empty() || sizeof(T) * m_vertexCount != m_vertexData.size()) {
      return vertices;
    }

    vertices.resize(m_vertexCount);
    std::memcpy(vertices.data(), m_vertexData.data(), m_vertexData.size());
    return vertices;
  }

  /**
   * Get vertex count
   *
   * @return Number of vertices
  */
  NODISCARD FORCEINLINE uint32
  getVertexCount() const { return m_vertexCount; }

  /**
   * Get index count
   *
   * @return Number of indices
   */
  NODISCARD FORCEINLINE uint32
  getIndexCount() const { return m_indexCount; }

  /**
   * Get index type
   *
   * @return Type of indices (16-bit or 32-bit)
   */
  NODISCARD FORCEINLINE IndexType
  getIndexType() const { return m_indexType; }

  /**
   * Get vertex layout
   *
   * @return Vertex layout
   */
  NODISCARD FORCEINLINE const VertexLayout&
  getVertexLayout() const { return m_vertexLayout; }

  /**
   * Check if the mesh has vertex data
   *
   * @return True if the mesh has vertex data
   */
  NODISCARD FORCEINLINE bool
  hasVertexData() const { return !m_vertexData.empty(); }

  /**
   * Check if the mesh has index data
   *
   * @return True if the mesh has index data
   */
  NODISCARD FORCEINLINE bool
  hasIndexData() const { return !m_indexData.empty(); }

  /**
   * Get size of vertex data in bytes
   *
   * @return Size of vertex data in bytes
   */
  NODISCARD FORCEINLINE SIZE_T
  getVertexDataSize() const { return m_vertexData.size(); }

  /**
   * Get size of index data in bytes
   *
   * @return Size of index data in bytes
   */
  NODISCARD FORCEINLINE SIZE_T
  getIndexDataSize() const { return m_indexData.size(); }

  const Vector<uint8>&
  getIndexData() const { return m_indexData; }

  /**
   * Bounds of the positions in the mesh's own space, computed on the first call after the
   * data changes. A mesh without positions gives zero bounds at the origin.
   */
  NODISCARD const SphereBoxBounds&
  getBounds() const;

  /**
   * GPU copy of the vertices, made on the first call after the data changes (the copy runs
   * with the next frame submit). Main thread only. Null when the mesh has no vertices.
   */
  NODISCARD const IBuffer*
  getVertexBuffer() const;

  /**
   * Same as getVertexBuffer, for the indices.
   */
  NODISCARD const IBuffer*
  getIndexBuffer() const;

  /**
   * Index into the material slots of the Model that holds this mesh.
   */
  FORCEINLINE void
  setMaterialSlot(uint32 slot) noexcept { m_materialSlot = slot; }

  NODISCARD FORCEINLINE uint32
  getMaterialSlot() const noexcept { return m_materialSlot; }

 private:
  /**
   * Old GPU buffers go through the deferred deletion, so frames in flight can still use
   * them.
   */
  FORCEINLINE void
  onDataChanged()
  {
    m_bBoundsDirty = true;
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
  }

  void
  createGpuBuffers() const;

  Vector<uint8> m_vertexData;
  Vector<uint8> m_indexData;
  uint32 m_vertexCount = 0;
  uint32 m_indexCount = 0;
  IndexType m_indexType = IndexType::UInt16;
  VertexLayout m_vertexLayout;
  uint32 m_materialSlot = 0;

  // Caches built on first use from the data above.
  mutable SphereBoxBounds m_bounds{Vector3::ZERO, Vector3::ZERO, 0.0f};
  mutable SPtr<IBuffer> m_vertexBuffer;
  mutable SPtr<IBuffer> m_indexBuffer;
  mutable bool m_bBoundsDirty = true;
};
} // namespace chEngineSDK
