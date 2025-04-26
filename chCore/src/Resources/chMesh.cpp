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

namespace chEngineSDK {
/*
*/
NODISCARD Vector<Vector3> 
Mesh::extractPositions() const {
  if (!hasVertexData() || m_vertexCount == 0) {
    return {};
  }
  
  bool foundPosition = false;
  uint32 positionOffset = 0;
  VertexFormat positionFormat = VertexFormat::Float3;
  
  for (const auto& attr : m_vertexLayout.getAttributes()) {
    if (attr.type == VertexAttributeType::Position) {
      positionOffset = attr.offset;
      positionFormat = attr.format;
      foundPosition = true;
      break;
    }
  }
  
  if (!foundPosition) {
    return {};
  }
  
  Vector<Vector3> positions;
  positions.reserve(m_vertexCount);
  
  uint32 vertexStride = m_vertexLayout.getVertexSize();
  
  for (uint32 i = 0; i < m_vertexCount; ++i) {
    const uint8* vertexPtr = m_vertexData.data() + (i * vertexStride) + positionOffset;
    
    Vector3 position;
    if (positionFormat == VertexFormat::Float3) {
      const float* posFloat = reinterpret_cast<const float*>(vertexPtr);
      position = Vector3(posFloat[0], posFloat[1], posFloat[2]);
    }
    else if (positionFormat == VertexFormat::Float2) {
      const float* posFloat = reinterpret_cast<const float*>(vertexPtr);
      position = Vector3(posFloat[0], posFloat[1], 0.0f);
    }
    positions.push_back(position);
  }
  
  return positions;
}
} // namespace chEngineSDK