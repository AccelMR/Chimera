/************************************************************************/
/**
 * @file chModelSerializer.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Writes and reads the data of a Model.
 */
/************************************************************************/
#include "chModelSerializer.h"

#include "chAssetFile.h"
#include "chFileStream.h"
#include "chLogger.h"
#include "chModel.h"

#include "chModelSerializationHeaders.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(ModelSerializerLog, All);

namespace {
// Above any real model; only a broken file asks for more.
constexpr uint32 kMaxNodes = 1u << 20;
constexpr uint32 kMaxMeshes = 1u << 20;
constexpr uint32 kMaxMaterialSlots = 4096;
constexpr uint32 kMaxNameLength = 1024;
constexpr uint32 kMaxAttributes = 32;
constexpr uint32 kMaxBindings = 16;

struct NodeFileData
{
  uint32 parent;
  float position[3];
  float rotation[4];
  float scale[3];
  uint32 meshCount;
  uint32 bone;
};
static_assert(sizeof(NodeFileData) == 52, "NodeFileData must have no padding");

/*
 * Checks the size against the bytes left before allocating, so a broken count cannot ask
 * for gigabytes.
 */
template<typename T>
NODISCARD bool
readArray(DataStream& stream, uint32 count, Vector<T>& outValues)
{
  const SIZE_T size = static_cast<SIZE_T>(count) * sizeof(T);
  if (size > AssetFile::getBytesLeft(stream)) {
    return false;
  }
  outValues.resize(count);
  return AssetFile::readBytes(stream, outValues.data(), size);
}

template<typename T>
void
writeArray(DataStream& stream, const Vector<T>& values)
{
  if (!values.empty()) {
    stream.write(values.data(), values.size() * sizeof(T));
  }
}

NODISCARD bool
readMaterialSlots(DataStream& stream, uint32 slotCount, Model& model)
{
  if (slotCount > kMaxMaterialSlots) {
    return false;
  }
  for (uint32 i = 0; i < slotCount; ++i) {
    ModelMaterialSlot slot;
    if (!AssetFile::readString(stream, slot.name, kMaxNameLength) ||
        !AssetFile::readValue(stream, slot.materialId)) {
      return false;
    }
    model.addMaterialSlot(std::move(slot));
  }
  return true;
}

NODISCARD bool
readVertexLayout(DataStream& stream, VertexLayout& outLayout)
{
  uint32 attributeCount = 0;
  if (!AssetFile::readValue(stream, attributeCount) || attributeCount > kMaxAttributes) {
    return false;
  }
  for (uint32 i = 0; i < attributeCount; ++i) {
    uint32 type = 0;
    uint32 format = 0;
    uint32 offset = 0;
    uint32 binding = 0;
    ANSICHAR semanticName[16];
    if (!AssetFile::readValue(stream, type) || !AssetFile::readValue(stream, format) ||
        !AssetFile::readValue(stream, offset) || !AssetFile::readValue(stream, binding) ||
        !AssetFile::readValue(stream, semanticName) ||
        type >= static_cast<uint32>(VertexAttributeType::COUNT) ||
        format >= static_cast<uint32>(VertexFormat::COUNT) || binding >= kMaxBindings) {
      return false;
    }
    semanticName[sizeof(semanticName) - 1] = '\0';

    const VertexAttributeType attributeType = static_cast<VertexAttributeType>(type);
    const VertexFormat attributeFormat = static_cast<VertexFormat>(format);
    if (attributeType == VertexAttributeType::Custom) {
      outLayout.addCustomAttribute(String(semanticName), attributeFormat, offset, binding);
    }
    else {
      outLayout.addAttribute(attributeType, attributeFormat, offset, binding);
    }
  }

  // The strides and the vertex size follow from the attributes; they are stored only so
  // the file can be read without the engine.
  uint32 bindingCount = 0;
  if (!AssetFile::readValue(stream, bindingCount) || bindingCount > kMaxBindings) {
    return false;
  }
  for (uint32 i = 0; i < bindingCount; ++i) {
    uint32 stride = 0;
    if (!AssetFile::readValue(stream, stride)) {
      return false;
    }
  }
  uint32 vertexSize = 0;
  return AssetFile::readValue(stream, vertexSize);
}

void
writeVertexLayout(DataStream& stream, const VertexLayout& layout)
{
  const Vector<VertexAttributeDesc>& attributes = layout.getAttributes();
  stream << static_cast<uint32>(attributes.size());
  for (const VertexAttributeDesc& attribute : attributes) {
    stream << static_cast<uint32>(attribute.type);
    stream << static_cast<uint32>(attribute.format);
    stream << attribute.offset;
    stream << attribute.binding;
    stream.write(attribute.semanticName, sizeof(attribute.semanticName));
  }

  const uint32 bindingCount = layout.getBindingCount();
  stream << bindingCount;
  for (uint32 i = 0; i < bindingCount; ++i) {
    stream << layout.getStride(i);
  }
  stream << layout.getVertexSize();
}

NODISCARD bool
readSkin(DataStream& stream, uint32 boneCount, uint32 vertexCount, MeshSkin& outSkin)
{
  if (boneCount > MeshSkin::MAX_BONES || !readArray(stream, boneCount, outSkin.boneNodes) ||
      !readArray(stream, boneCount, outSkin.inverseBindMatrices) ||
      !readArray(stream, vertexCount, outSkin.vertices)) {
    return false;
  }
  for (const VertexSkin& vertex : outSkin.vertices) {
    for (uint32 i = 0; i < MeshSkin::MAX_INFLUENCES; ++i) {
      if (vertex.weights[i] > 0 && vertex.bones[i] >= boneCount) {
        return false;
      }
    }
  }
  return true;
}
} // namespace

/*
 */
void
ModelSerializer::write(DataStream& stream, const Model& model)
{
  const Vector<ModelMaterialSlot>& slots = model.getMaterialSlots();
  const ModelHeader header{.version = ModelHeader::VERSION,
                           .nodeCount = model.getNodeCount(),
                           .meshCount = model.getMeshCount(),
                           .materialSlotCount = static_cast<uint32>(slots.size())};
  stream << header;

  for (const ModelMaterialSlot& slot : slots) {
    AssetFile::writeString(stream, slot.name);
    stream << slot.materialId;
  }
  stream << model.getSkeletonId();

  for (const SPtr<Mesh>& mesh : model.getMeshes()) {
    writeMesh(stream, *mesh);
  }

  const Vector<ModelNode>& nodes = model.getNodes();
  for (uint32 nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
    const ModelNode& node = nodes[nodeIndex];
    const NodeFileData data{
        .parent = node.parent,
        .position = {node.position.x, node.position.y, node.position.z},
        .rotation = {node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w},
        .scale = {node.scale.x, node.scale.y, node.scale.z},
        .meshCount = node.meshCount,
        .bone = node.bone};
    AssetFile::writeString(stream, node.name);
    stream << data;
    const Span<const uint32> meshIndices = model.getNodeMeshes(nodeIndex);
    if (!meshIndices.empty()) {
      stream.write(meshIndices.data(), meshIndices.size_bytes());
    }
  }
}

/*
 */
SPtr<Model>
ModelSerializer::read(DataStream& stream)
{
  ModelHeader header;
  if (!AssetFile::readValue(stream, header)) {
    return nullptr;
  }
  if (header.version != ModelHeader::VERSION) {
    CH_LOG_ERROR(ModelSerializerLog, "Unsupported model version {0}, import the model again",
                 header.version);
    return nullptr;
  }
  if (header.nodeCount > kMaxNodes || header.meshCount > kMaxMeshes) {
    return nullptr;
  }

  SPtr<Model> model = chMakeShared<Model>();
  UUID skeletonId = UUID::null();
  if (!readMaterialSlots(stream, header.materialSlotCount, *model) ||
      !AssetFile::readValue(stream, skeletonId)) {
    return nullptr;
  }
  model->setSkeleton(skeletonId, nullptr);

  for (uint32 i = 0; i < header.meshCount; ++i) {
    SPtr<Mesh> mesh = readMesh(stream);
    if (!mesh) {
      return nullptr;
    }
    // The nodes come later, so the bones of a skin are checked against their count.
    for (const uint32 boneNode : mesh->getSkin().boneNodes) {
      if (boneNode >= header.nodeCount) {
        return nullptr;
      }
    }
    model->addMesh(std::move(mesh));
  }

  String name;
  Vector<uint32> meshIndices;
  for (uint32 nodeIndex = 0; nodeIndex < header.nodeCount; ++nodeIndex) {
    NodeFileData data;
    if (!AssetFile::readString(stream, name, kMaxNameLength) ||
        !AssetFile::readValue(stream, data) ||
        (data.parent != ModelNode::NO_PARENT && data.parent >= nodeIndex) ||
        !readArray(stream, data.meshCount, meshIndices)) {
      return nullptr;
    }
    for (const uint32 meshIndex : meshIndices) {
      if (meshIndex >= header.meshCount) {
        return nullptr;
      }
    }
    model->addNode(
        name, data.parent, Vector3(data.position[0], data.position[1], data.position[2]),
        Quaternion(data.rotation[0], data.rotation[1], data.rotation[2], data.rotation[3]),
        Vector3(data.scale[0], data.scale[1], data.scale[2]), meshIndices);
    model->setNodeBone(nodeIndex, data.bone);
  }
  return model;
}

/*
 */
void
ModelSerializer::writeMesh(DataStream& stream, const Mesh& mesh)
{
  const VertexLayout& layout = mesh.getVertexLayout();
  const MeshSkin& skin = mesh.getSkin();
  const MeshHeader header{
      .version = MeshHeader::VERSION,
      .vertexCount = mesh.getVertexCount(),
      .indexCount = mesh.getIndexCount(),
      .vertexDataSize = static_cast<uint32>(mesh.getVertexDataSize()),
      .indexDataSize = static_cast<uint32>(mesh.getIndexDataSize()),
      .attributeCount = static_cast<uint32>(layout.getAttributes().size()),
      .indexType = mesh.getIndexType(),
      .vertexStride = layout.getVertexSize(),
      .materialSlot = mesh.getMaterialSlot(),
      .skinBoneCount = static_cast<uint32>(skin.boneNodes.size())};
  stream << header;
  writeVertexLayout(stream, layout);
  writeArray(stream, mesh.getVertexData());
  writeArray(stream, mesh.getIndexData());
  if (header.skinBoneCount > 0) {
    writeArray(stream, skin.boneNodes);
    writeArray(stream, skin.inverseBindMatrices);
    writeArray(stream, skin.vertices);
  }
}

/*
 */
SPtr<Mesh>
ModelSerializer::readMesh(DataStream& stream)
{
  MeshHeader header;
  if (!AssetFile::readValue(stream, header)) {
    return nullptr;
  }
  if (header.version != MeshHeader::VERSION) {
    CH_LOG_ERROR(ModelSerializerLog, "Unsupported mesh version {0}, import the model again",
                 header.version);
    return nullptr;
  }

  VertexLayout layout;
  if (!readVertexLayout(stream, layout) ||
      header.attributeCount != layout.getAttributes().size() ||
      static_cast<uint64>(header.vertexCount) * layout.getVertexSize() !=
          header.vertexDataSize) {
    return nullptr;
  }

  if (header.indexType != IndexType::UInt16 && header.indexType != IndexType::UInt32) {
    return nullptr;
  }
  const uint64 indexSize = header.indexType == IndexType::UInt16 ? 2 : 4;
  if (static_cast<uint64>(header.indexCount) * indexSize != header.indexDataSize) {
    return nullptr;
  }

  Vector<uint8> vertexData;
  Vector<uint8> indexData;
  MeshSkin skin;
  if (!readArray(stream, header.vertexDataSize, vertexData) ||
      !readArray(stream, header.indexDataSize, indexData) ||
      (header.skinBoneCount > 0 &&
       !readSkin(stream, header.skinBoneCount, header.vertexCount, skin))) {
    return nullptr;
  }

  SPtr<Mesh> mesh = chMakeShared<Mesh>();
  mesh->setVertexLayout(layout);
  mesh->setVertexData(std::move(vertexData), header.vertexCount);
  mesh->setIndexData(std::move(indexData), header.indexCount, header.indexType);
  mesh->setMaterialSlot(header.materialSlot);
  mesh->setSkin(std::move(skin));
  return mesh;
}

} // namespace chEngineSDK
