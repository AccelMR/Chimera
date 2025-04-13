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

namespace chEngineSDK {
enum class VertexAttributeType {
  Position,
  Normal,
  Color,
  TexCoord0,
  TexCoord1,
  Tangent,
  Bitangent,
  BoneIndices,
  BoneWeights,
  Custom,

  COUNT
};

enum class VertexFormat {
  Float,
  Float2,
  Float3,
  Float4,
  Int,
  Int2,
  Int3,
  Int4,
  UInt,
  UInt2,
  UInt3,
  UInt4,
  Byte4,
  Byte4Normalized,
  UByte4,
  UByte4Normalized,
  Short2,
  Short2Normalized,
  Short4,
  Short4Normalized,

  COUNT
};

struct VertexAttributeDesc {
  VertexAttributeType type;
  VertexFormat format;
  uint32 offset;  // offset in bytes from the start of the vertex structure
  uint32 binding; // to support multiple vertex buffers
  String semanticName; // For custom attributes
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
  static VertexLayout createPositionNormalTexCoordLayout();

 private:
  Vector<VertexAttributeDesc> m_attributes;
  Vector<uint32> m_strides;   // Tamaño en bytes de cada binding
  uint32 m_vertexSize = 0;   // Tamaño total de un vértice en el binding 0
};


} // namespace chEngineSDK