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

#include "chGraphicsTypes.h"
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
  }

  /**
   * Set the vertex layout for the mesh
   *
   * @param layout Vertex layout
  */
  FORCEINLINE void
  setVertexLayout(const VertexLayout& layout) { m_vertexLayout = layout; }

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
   * Get indices as uint16
   *
   * @return Vector of indices as uint16
  */
  NODISCARD Vector<uint16>
  getIndicesAsUInt16() const {
    Vector<uint16> result;
    if (m_indexType != IndexType::UInt16 || m_indexData.empty()) {
      return result;
    }

    result.resize(m_indexCount);
    std::memcpy(result.data(), m_indexData.data(), m_indexData.size());
    return result;
  }

  /**
   * Get indices as uint32
   *
   * @return Vector of indices as uint32
  */
  NODISCARD Vector<uint32>
  getIndicesAsUInt32() const {
    Vector<uint32> result;
    if (m_indexType != IndexType::UInt32 || m_indexData.empty()) {
      return result;
    }

    result.resize(m_indexCount);
    std::memcpy(result.data(), m_indexData.data(), m_indexData.size());
    return result;
  }


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
   * Extract all vertex positions from the mesh
   *
   * @return Vector of position vectors
   */
  NODISCARD Vector<Vector3>
  extractPositions() const;

 private:
  Vector<uint8> m_vertexData;
  Vector<uint8> m_indexData;
  uint32 m_vertexCount = 0;
  uint32 m_indexCount = 0;
  IndexType m_indexType = IndexType::UInt16;
  VertexLayout m_vertexLayout;
};
} // namespace chEngineSDK
