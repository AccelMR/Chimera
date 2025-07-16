/************************************************************************/
/**
 * @file chModelSerializationHeaders.h
 * @author AccelMR
 * @date 2025/07/14
 * @brief  Model header structures for serialization.
 *         This file contains the structures used for serializing model data.
 */
/***********************************************************************/
#pragma once

#include "chGraphicsTypes.h"

namespace chEngineSDK {
// Model header structure
struct ModelHeader {
  uint32 version;
  uint32 nodeCount;
  uint32 uniqueMeshCount;
};

// Mesh header structure
struct MeshHeader {
  uint32 version;
  uint32 vertexCount;
  uint32 indexCount;
  uint32 vertexDataSize;
  uint32 indexDataSize;
  uint32 attributeCount;
  IndexType indexType;
  uint32 vertexStride;
};
} // namespace chEngineSDK
