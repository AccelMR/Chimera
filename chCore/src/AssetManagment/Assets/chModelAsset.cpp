/************************************************************************/
/**
 * @file chModelAsset.cpp
 * @author AccelMR
 * @date 2025/07/14
 * @brief
 *  This file contains the ModelAsset class for managing model assets in the engine.
 */
/************************************************************************/
#include "chModelAsset.h"

#include "chFileStream.h"
#include "chLogger.h"
#include "chMesh.h"
#include "chModel.h"

#include "chModelSerializationHeaders.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(ModelAssetLog, All);
// Implementation for ModelAsset::serialize() method in chModelAsset.cpp

bool
ModelAsset::serialize(SPtr<DataStream> stream) {
  if (!stream) {
    CH_LOG(ModelAssetLog, Error, "Data stream is null for ModelAsset serialization");
    return false;
  }

  if (!m_model) {
    CH_LOG(ModelAssetLog, Error, "Model is null for ModelAsset {0}", m_metadata.name);
    return false;
  }

  try {
    // Write Model Header
    ModelHeader modelHeader = {
        .version = 1,
        .nodeCount = m_model->getNodeCount(),
        .uniqueMeshCount = static_cast<uint32>(m_model->getMeshToNodesMap().size())
    };

    stream << modelHeader;

    // Serialize global model transform
    const Matrix4& globalTransform = m_model->getTransform();
    stream << globalTransform;

    serializeUniqueMeshes(stream);
    serializeNodeTree(stream);

    CH_LOG(ModelAssetLog, Debug,
           "ModelAsset {0} serialized successfully with {1} nodes and {2} unique meshes",
           m_metadata.name, modelHeader.nodeCount, modelHeader.uniqueMeshCount);

    return true;
  }
  catch (const std::exception& e) {
    CH_LOG(ModelAssetLog, Error, "Exception during ModelAsset serialization: {0}", e.what());
    return false;
  }
}

void
ModelAsset::serializeNodeTree(SPtr<DataStream> stream) {
  if (!stream || !m_model) {
    return;
  }

  // Create mesh index mapping for efficient storage
  UnorderedMap<SPtr<Mesh>, uint32> meshToIndexMap;
  uint32 meshIndex = 0;
  for (const auto& [mesh, nodes] : m_model->getMeshToNodesMap()) {
    meshToIndexMap[mesh] = meshIndex++;
  }

  // Serialize each root node recursively
  const Vector<ModelNode*>& rootNodes = m_model->getRootNodes();
  stream << static_cast<uint32>(rootNodes.size());

  for (ModelNode* rootNode : rootNodes) {
    serializeNode(stream, rootNode, meshToIndexMap);
  }
}

void
ModelAsset::serializeNode(SPtr<DataStream> stream, ModelNode* node,
                          const UnorderedMap<SPtr<Mesh>, uint32>& meshToIndexMap) {
  if (!stream || !node) {
    return;
  }

  // Node basic data
  stream->write(node->getName(), 64);
  stream << node->getLocalTransform();
  stream << node->getGlobalTransform();

  // Serialize meshes (as indices to unique mesh array)
  const Vector<SPtr<Mesh>>& meshes = node->getMeshes();
  stream << static_cast<uint32>(meshes.size());

  for (const SPtr<Mesh>& mesh : meshes) {
    auto it = meshToIndexMap.find(mesh);
    if (it != meshToIndexMap.end()) {
      stream << it->second; // Write mesh index
    }
    else {
      CH_LOG(ModelAssetLog, Warning, "Mesh not found in index mapping for node {0}",
             node->getName());
      stream << static_cast<uint32>(-1); // Write invalid index
    }
  }

  // Serialize children recursively
  const Vector<ModelNode*>& children = node->getChildren();
  stream << static_cast<uint32>(children.size());

  for (ModelNode* child : children) {
    serializeNode(stream, child, meshToIndexMap);
  }
}

void
ModelAsset::serializeUniqueMeshes(SPtr<DataStream> stream) {
  if (!stream || !m_model) {
    return;
  }

  const auto& meshToNodesMap = m_model->getMeshToNodesMap();

  // Write number of unique meshes
  stream << static_cast<uint32>(meshToNodesMap.size());

  // Serialize each unique mesh
  for (const auto& [mesh, nodes] : meshToNodesMap) {
    serializeMesh(stream, mesh);
  }
}

void
ModelAsset::serializeMesh(SPtr<DataStream> stream, SPtr<Mesh> mesh) {
  if (!stream || !mesh) {
    return;
  }

  // Create and write mesh header
  MeshHeader meshHeader = {
      .version = 1,
      .vertexCount = mesh->getVertexCount(),
      .indexCount = mesh->getIndexCount(),
      .vertexDataSize = static_cast<uint32>(mesh->getVertexDataSize()),
      .indexDataSize = static_cast<uint32>(mesh->getIndexDataSize()),
      .attributeCount = static_cast<uint32>(mesh->getVertexLayout().getAttributes().size()),
      .indexType = mesh->getIndexType(),
      .vertexStride = mesh->getVertexLayout().getVertexSize()};

  stream << meshHeader;

  // Serialize VertexLayout
  serializeVertexLayout(stream, mesh->getVertexLayout());

  // Write vertex data
  if (meshHeader.vertexDataSize > 0) {
    const Vector<uint8>& vertexData = mesh->getVertexData();
    stream->write(vertexData.data(), meshHeader.vertexDataSize);
  }

  // Write index data
  if (meshHeader.indexDataSize > 0) {
    const Vector<uint8>& indexData = mesh->getIndexData();
    stream->write(indexData.data(), meshHeader.indexDataSize);
  }
}

void
ModelAsset::serializeVertexLayout(SPtr<DataStream> stream, const VertexLayout& layout) {
  if (!stream) {
    return;
  }

  const Vector<VertexAttributeDesc>& attributes = layout.getAttributes();

  // Write number of attributes (redundant but good for validation)
  stream << static_cast<uint32>(attributes.size());

  // Write each attribute descriptor
  for (const VertexAttributeDesc& attr : attributes) {
    stream << static_cast<uint32>(attr.type);
    stream << static_cast<uint32>(attr.format);
    stream << attr.offset;
    stream << attr.binding;

    // Write semantic name (16 bytes fixed size)
    stream->write(attr.semanticName, sizeof(attr.semanticName));
  }

  // Write stride information for each binding
  uint32 bindingCount = layout.getBindingCount();
  stream << bindingCount;

  for (uint32 i = 0; i < bindingCount; ++i) {
    stream << layout.getStride(i);
  }

  // Write total vertex size
  stream << layout.getVertexSize();
}

bool
ModelAsset::deserialize(SPtr<DataStream> stream) {
  if (!stream) {
    CH_LOG(ModelAssetLog, Error, "Data stream is null for ModelAsset deserialization");
    return false;
  }

  try {
    // Read Model Header
    ModelHeader modelHeader;
    stream >> modelHeader;

    if (modelHeader.version != 1) {
      CH_LOG(ModelAssetLog, Error, "Unsupported model version: {0}", modelHeader.version);
      return false;
    }

    // Create new model
    m_model = chMakeShared<Model>();

    // Read global model transform
    Matrix4 globalTransform;
    stream >> globalTransform;
    m_model->setTransform(globalTransform);

    // Read unique meshes first (they'll be referenced by nodes)
    Vector<SPtr<Mesh>> uniqueMeshes;
    if (!deserializeUniqueMeshes(stream, uniqueMeshes, modelHeader.uniqueMeshCount)) {
      CH_LOG(ModelAssetLog, Error, "Failed to deserialize meshes for ModelAsset {0}",
             m_metadata.name);
      return false;
    }

    // Read node tree structure
    if (!deserializeNodeTree(stream, uniqueMeshes)) {
      CH_LOG(ModelAssetLog, Error, "Failed to deserialize node tree for ModelAsset {0}",
             m_metadata.name);
      return false;
    }

    // Validate deserialized data
    if (m_model->getNodeCount() != modelHeader.nodeCount) {
      CH_LOG(ModelAssetLog, Warning, "Node count mismatch. Expected: {0}, Got: {1}",
             modelHeader.nodeCount, m_model->getNodeCount());
    }

    // Update all transforms
    m_model->updateTransforms();

    CH_LOG(ModelAssetLog, Debug,
           "ModelAsset {0} deserialized successfully with {1} nodes and {2} unique meshes",
           m_metadata.name, m_model->getNodeCount(), uniqueMeshes.size());

    return true;
  } catch (const std::exception& e) {
    CH_LOG(ModelAssetLog, Error, "Exception during ModelAsset deserialization: {0}", e.what());
    m_model.reset();
    return false;
  }
}

bool
ModelAsset::deserializeNodeTree(SPtr<DataStream> stream,
                                const Vector<SPtr<Mesh>>& uniqueMeshes) {
  if (!stream || !m_model) {
    return false;
  }

  // Read number of root nodes
  uint32 rootNodeCount;
  stream >> rootNodeCount;

  // Deserialize each root node recursively
  for (uint32 i = 0; i < rootNodeCount; ++i) {
    if (!deserializeNode(stream, nullptr, uniqueMeshes)) {
      return false;
    }
  }

  return true;
}

bool
ModelAsset::deserializeNode(SPtr<DataStream> stream, ModelNode* parent,
                            const Vector<SPtr<Mesh>>& uniqueMeshes) {
  if (!stream || !m_model) {
    return false;
  }

  // Read node basic data
  ANSICHAR nodeName[64];
  Matrix4 localTransform, globalTransform;

  stream >> nodeName;
  stream >> localTransform;
  stream >> globalTransform;

  // Create the node
  ModelNode* node = m_model->createNode(nodeName, localTransform, parent);
  if (!node) {
    CH_LOG(ModelAssetLog, Error, "Failed to create node: {0}", nodeName);
    return false;
  }

  // Read mesh indices and assign meshes
  uint32 meshCount;
  stream >> meshCount;

  for (uint32 i = 0; i < meshCount; ++i) {
    uint32 meshIndex;
    stream >> meshIndex;

    if (meshIndex < uniqueMeshes.size()) {
      node->addMesh(uniqueMeshes[meshIndex]);
    }
    else {
      CH_LOG(ModelAssetLog, Warning, "Invalid mesh index {0} for node {1}", meshIndex,
             nodeName);
    }
  }

  // Read and deserialize children recursively
  uint32 childCount;
  stream >> childCount;

  for (uint32 i = 0; i < childCount; ++i) {
    if (!deserializeNode(stream, node, uniqueMeshes)) {
      return false;
    }
  }

  return true;
}

bool
ModelAsset::deserializeUniqueMeshes(SPtr<DataStream> stream, Vector<SPtr<Mesh>>& uniqueMeshes,
                                    uint32 expectedMeshCount) {
  if (!stream) {
    return false;
  }

  // Read number of unique meshes
  uint32 meshCount;
  stream >> meshCount;

  if (meshCount != expectedMeshCount) {
    CH_LOG(ModelAssetLog, Warning, "Mesh count mismatch. Expected: {0}, Got: {1}",
           expectedMeshCount, meshCount);
  }

  uniqueMeshes.reserve(meshCount);

  // Deserialize each unique mesh
  for (uint32 i = 0; i < meshCount; ++i) {
    SPtr<Mesh> mesh = chMakeShared<Mesh>();

    if (!deserializeMesh(stream, mesh)) {
      CH_LOG(ModelAssetLog, Error, "Failed to deserialize mesh {0}", i);
      return false;
    }

    uniqueMeshes.push_back(mesh);
  }

  return true;
}

bool
ModelAsset::deserializeMesh(SPtr<DataStream> stream, SPtr<Mesh> mesh) {
  if (!stream || !mesh) {
    return false;
  }

  // Read mesh header
  MeshHeader meshHeader;
  stream >> meshHeader;

  if (meshHeader.version != 1) {
    CH_LOG(ModelAssetLog, Error, "Unsupported mesh version: {0}", meshHeader.version);
    return false;
  }

  // Deserialize VertexLayout
  VertexLayout layout;
  if (!deserializeVertexLayout(stream, layout, meshHeader.attributeCount)) {
    CH_LOG(ModelAssetLog, Error, "Failed to deserialize vertex layout");
    return false;
  }

  mesh->setVertexLayout(layout);

  // Read vertex data
  if (meshHeader.vertexDataSize > 0) {
    Vector<uint8> vertexData(meshHeader.vertexDataSize);
    stream->read(vertexData.data(), meshHeader.vertexDataSize);
    mesh->setVertexData(vertexData, meshHeader.vertexCount);
  }

  // Read index data
  if (meshHeader.indexDataSize > 0) {
    Vector<uint8> indexData(meshHeader.indexDataSize);
    stream->read(indexData.data(), meshHeader.indexDataSize);

    // Set index data based on type
    setMeshIndexDataRaw(mesh, indexData, meshHeader.indexCount, meshHeader.indexType);
  }

  return true;
}

bool
ModelAsset::deserializeVertexLayout(SPtr<DataStream> stream, VertexLayout& layout,
                                    uint32 expectedAttributeCount) {
  if (!stream) {
    return false;
  }

  // Read number of attributes
  uint32 attributeCount;
  stream >> attributeCount;

  if (attributeCount != expectedAttributeCount) {
    CH_LOG(ModelAssetLog, Warning, "Attribute count mismatch. Expected: {0}, Got: {1}",
           expectedAttributeCount, attributeCount);
  }

  // Read each attribute descriptor
  for (uint32 i = 0; i < attributeCount; ++i) {
    uint32 type, format;
    uint32 offset, binding;
    ANSICHAR semanticName[16];

    stream >> type;
    stream >> format;
    stream >> offset;
    stream >> binding;
    stream->read(semanticName, sizeof(semanticName));

    VertexAttributeType attrType = static_cast<VertexAttributeType>(type);
    VertexFormat attrFormat = static_cast<VertexFormat>(format);

    if (attrType == VertexAttributeType::Custom) {
      layout.addCustomAttribute(String(semanticName), attrFormat, offset, binding);
    }
    else {
      layout.addAttribute(attrType, attrFormat, offset, binding);
    }
  }

  // Read stride information (for validation)
  uint32 bindingCount;
  stream >> bindingCount;

  for (uint32 i = 0; i < bindingCount; ++i) {
    uint32 stride;
    stream >> stride;
    // Note: VertexLayout calculates strides automatically, so this is mainly for validation
  }

  // Read total vertex size (for validation)
  uint32 vertexSize;
  stream >> vertexSize;

  return true;
}

void
ModelAsset::setMeshIndexDataRaw(SPtr<Mesh> mesh, const Vector<uint8>& data, uint32 indexCount,
                                IndexType indexType) {
  // Set index data based on type
  if (indexType == IndexType::UInt16) {
    Vector<uint16> indices(indexCount);
    std::memcpy(indices.data(), data.data(), data.size());
    mesh->setIndexData(indices);
  }
  else if (indexType == IndexType::UInt32) {
    Vector<uint32> indices(indexCount);
    std::memcpy(indices.data(), data.data(), data.size());
    mesh->setIndexData(indices);
  }
}

} // namespace chEngineSDK
