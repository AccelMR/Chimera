/************************************************************************/
/**
 * @file chVertexLayout.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vertex layout class. This class is used to define the vertex layout
 * for the graphics pipeline. It is used to describe the format of the
 * vertex data and how it is laid out in memory.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chLinearColor.h"
#include "chVector2.h"
#include "chVector3.h"
#include "chVector4.h"

namespace chEngineSDK {
/**
 * What a vertex attribute holds. Mesh files store the value, so every value is fixed: new
 * types go at the end and none is ever reordered.
 */
enum class VertexAttributeType : uint32 {
  Position = 0,
  Normal = 1,
  Color = 2,
  TexCoord0 = 3,
  TexCoord1 = 4,
  Tangent = 5,
  Bitangent = 6,
  BoneIndices = 7,
  BoneWeights = 8,
  Custom = 9,

  COUNT
};

/**
 * Data format of a vertex attribute. Mesh files store the value, so every value is fixed:
 * new formats go at the end and none is ever reordered.
 */
enum class VertexFormat : uint32 {
  Float = 0,
  Float2 = 1,
  Float3 = 2,
  Float4 = 3,
  Int = 4,
  Int2 = 5,
  Int3 = 6,
  Int4 = 7,
  UInt = 8,
  UInt2 = 9,
  UInt3 = 10,
  UInt4 = 11,
  Byte4 = 12,
  Byte4Normalized = 13,
  UByte4 = 14,
  UByte4Normalized = 15,
  Short2 = 16,
  Short2Normalized = 17,
  Short4 = 18,
  Short4Normalized = 19,

  COUNT
};

struct VertexAttributeDesc {
  VertexAttributeType type;      // 4 bytes
  VertexFormat format;           // 4 bytes
  uint32 offset;                 // 4 bytes
  uint32 binding;                // 4 bytes
  ANSICHAR semanticName[16];     // 16 bytes
  // Total: 32 bytes, aligned to 4 bytes
};

class CH_CORE_EXPORT VertexLayout {
 public:
  VertexLayout() = default;
  ~VertexLayout() = default;

  void
  addAttribute(VertexAttributeType type,
               VertexFormat format,
               uint32 offset = UINT32_MAX,
               uint32 binding = 0);

  void
  addCustomAttribute(const String& semanticName,
                     VertexFormat format,
                     uint32 offset,
                     uint32 binding = 0);

  NODISCARD FORCEINLINE const Vector<VertexAttributeDesc>&
  getAttributes() const { return m_attributes; }

  NODISCARD FORCEINLINE uint32
  getVertexSize() const { return m_vertexSize; }

  NODISCARD FORCEINLINE uint32
  getStride(uint32 binding = 0) const {
    return binding < m_strides.size() ? m_strides[binding] : 0;
  }

  NODISCARD FORCEINLINE uint32
  getBindingCount() const {
    return static_cast<uint32>(m_strides.size());
  }

 private:

  uint32
  calculateOffset(uint32 binding) const;

  void
  updateSizes(uint32 binding, uint32 offset, VertexFormat format);

  static uint32
  getFormatSize(VertexFormat format);

 public:
  static VertexLayout createPostionColorLayout();
  static VertexLayout createStaticLayout();

 private:
  Vector<VertexAttributeDesc> m_attributes;
  Vector<uint32> m_strides;   // Tamaño en bytes de cada binding
  uint32 m_vertexSize = 0;   // Tamaño total de un vértice en el binding 0
};

/*
 * Vertex structures for different vertex layouts.
 * These structures are used to define the vertex data format
 * for the graphics pipeline.
*/

struct VertexPosColor {
  Vector3 position;
  LinearColor color;

  static VertexLayout getLayout() {
    return VertexLayout::createPostionColorLayout();
  }
};

struct VertexStatic {
  Vector3 position;
  Vector3 normal;
  Vector2 texCoord;
  // xyz along the texture U; w gives the bitangent, cross(normal, tangent) * w, which points
  // up the image (towards smaller V).
  Vector4 tangent;

  static VertexLayout
  getLayout()
  {
    return VertexLayout::createStaticLayout();
  }
};
} // namespace chEngineSDK
