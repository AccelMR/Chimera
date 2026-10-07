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
  static constexpr uint32 VERSION = 1;

  uint32 version;
  uint32 nodeCount;
  uint32 uniqueMeshCount;
  uint32 materialSlotCount;
};
static_assert(sizeof(ModelHeader) == 16, "ModelHeader must have no padding");

// Mesh header structure
struct MeshHeader {
  static constexpr uint32 VERSION = 1;

  uint32 version;
  uint32 vertexCount;
  uint32 indexCount;
  uint32 vertexDataSize;
  uint32 indexDataSize;
  uint32 attributeCount;
  IndexType indexType;
  uint32 vertexStride;
  uint32 materialSlot;
};
static_assert(sizeof(MeshHeader) == 36, "MeshHeader must have no padding");
} // namespace chEngineSDK
