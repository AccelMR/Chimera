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
#include "chMatrixHelpers.h"
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
};
static_assert(sizeof(NodeFileData) == 48, "NodeFileData must have no padding");

template<typename T>
NODISCARD bool
readValue(DataStream& stream, T& outValue)
{
  return stream.read(&outValue, sizeof(T)) == sizeof(T);
}

/*
 * Bytes left to read, so a stored size can be checked before anything is allocated for it.
 */
NODISCARD SIZE_T
getBytesLeft(const DataStream& stream)
{
  const SIZE_T position = stream.tell();
  return stream.size() > position ? stream.size() - position : 0;
}

NODISCARD bool
readBytes(DataStream& stream, SIZE_T size, Vector<uint8>& outBytes)
{
  if (size > getBytesLeft(stream)) {
    return false;
  }
  outBytes.resize(size);
  return size == 0 || stream.read(outBytes.data(), size) == size;
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
        !readValue(stream, slot.materialId)) {
      return false;
    }
    model.addMaterialSlot(std::move(slot));
  }
  return true;
}

NODISCARD bool
readMeshIndices(DataStream& stream, uint32 meshCount, uint32 modelMeshCount,
                Vector<uint32>& outIndices)
{
  if (meshCount > kMaxMeshes || meshCount * sizeof(uint32) > getBytesLeft(stream)) {
    return false;
  }
  outIndices.resize(meshCount);
  if (meshCount > 0 &&
      stream.read(outIndices.data(), meshCount * sizeof(uint32)) != meshCount * sizeof(uint32)) {
    return false;
  }
  for (const uint32 index : outIndices) {
    if (index >= modelMeshCount) {
      return false;
    }
  }
  return true;
}

NODISCARD bool
readVertexLayout(DataStream& stream, VertexLayout& outLayout)
{
  uint32 attributeCount = 0;
  if (!readValue(stream, attributeCount) || attributeCount > kMaxAttributes) {
    return false;
  }
  for (uint32 i = 0; i < attributeCount; ++i) {
    uint32 type = 0;
    uint32 format = 0;
    uint32 offset = 0;
    uint32 binding = 0;
    ANSICHAR semanticName[16];
    if (!readValue(stream, type) || !readValue(stream, format) ||
        !readValue(stream, offset) || !readValue(stream, binding) ||
        !readValue(stream, semanticName) ||
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
  if (!readValue(stream, bindingCount) || bindingCount > kMaxBindings) {
    return false;
  }
  for (uint32 i = 0; i < bindingCount; ++i) {
    uint32 stride = 0;
    if (!readValue(stream, stride)) {
      return false;
    }
  }
  uint32 vertexSize = 0;
  return readValue(stream, vertexSize);
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
        .meshCount = node.meshCount};
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
  if (!readValue(stream, header)) {
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
  if (!readMaterialSlots(stream, header.materialSlotCount, *model)) {
    return nullptr;
  }
  for (uint32 i = 0; i < header.meshCount; ++i) {
    SPtr<Mesh> mesh = readMesh(stream);
    if (!mesh) {
      return nullptr;
    }
    model->addMesh(std::move(mesh));
  }

  String name;
  Vector<uint32> meshIndices;
  for (uint32 nodeIndex = 0; nodeIndex < header.nodeCount; ++nodeIndex) {
    NodeFileData data;
    if (!AssetFile::readString(stream, name, kMaxNameLength) || !readValue(stream, data) ||
        (data.parent != ModelNode::NO_PARENT && data.parent >= nodeIndex) ||
        !readMeshIndices(stream, data.meshCount, header.meshCount, meshIndices)) {
      return nullptr;
    }
    model->addNode(
        name, data.parent, Vector3(data.position[0], data.position[1], data.position[2]),
        Quaternion(data.rotation[0], data.rotation[1], data.rotation[2], data.rotation[3]),
        Vector3(data.scale[0], data.scale[1], data.scale[2]), meshIndices);
  }
  return model;
}

/*
 */
void
ModelSerializer::writeMesh(DataStream& stream, const Mesh& mesh)
{
  const VertexLayout& layout = mesh.getVertexLayout();
  const MeshHeader header{
      .version = MeshHeader::VERSION,
      .vertexCount = mesh.getVertexCount(),
      .indexCount = mesh.getIndexCount(),
      .vertexDataSize = static_cast<uint32>(mesh.getVertexDataSize()),
      .indexDataSize = static_cast<uint32>(mesh.getIndexDataSize()),
      .attributeCount = static_cast<uint32>(layout.getAttributes().size()),
      .indexType = mesh.getIndexType(),
      .vertexStride = layout.getVertexSize(),
      .materialSlot = mesh.getMaterialSlot()};
  stream << header;
  writeVertexLayout(stream, layout);
  if (header.vertexDataSize > 0) {
    stream.write(mesh.getVertexData().data(), header.vertexDataSize);
  }
  if (header.indexDataSize > 0) {
    stream.write(mesh.getIndexData().data(), header.indexDataSize);
  }
}

/*
 */
SPtr<Mesh>
ModelSerializer::readMesh(DataStream& stream)
{
  MeshHeader header;
  if (!readValue(stream, header)) {
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
  if (!readBytes(stream, header.vertexDataSize, vertexData) ||
      !readBytes(stream, header.indexDataSize, indexData)) {
    return nullptr;
  }

  SPtr<Mesh> mesh = chMakeShared<Mesh>();
  mesh->setVertexLayout(layout);
  mesh->setVertexData(std::move(vertexData), header.vertexCount);
  mesh->setIndexData(std::move(indexData), header.indexCount, header.indexType);
  mesh->setMaterialSlot(header.materialSlot);
  return mesh;
}

} // namespace chEngineSDK
