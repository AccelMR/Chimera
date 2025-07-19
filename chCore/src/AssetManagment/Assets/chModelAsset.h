/************************************************************************/
/**
 * @file chModelAsset.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 *  This file contains the ModelAsset class for managing model assets in the engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"

namespace chEngineSDK {
class Model;
class ModelNode;
class Mesh;
class VertexLayout;
enum class IndexType : uint32;

class CH_CORE_EXPORT ModelAsset : public IAsset
{
 public:
  ModelAsset() = delete;
  ModelAsset(const AssetMetadata& metadata) : IAsset(metadata) {}

  ~ModelAsset() = default;

  NODISCARD FORCEINLINE SPtr<Model>
  getModel() const {
    return m_model;
  }

 protected:
  bool
  serialize(SPtr<DataStream>) override;

 /**
  * Deserialize the model asset from a data stream
  *
  * @param stream Data stream to read from
  * @return true if successful, false otherwise
  */
  bool
  deserialize(SPtr<DataStream>) override;

  friend class MeshImpotrter;
  void
  clearAssetData() override;

  FORCEINLINE void
  setModel(const SPtr<Model>& model) {
    m_model = model;
  }

  void
  serializeNodeTree(SPtr<DataStream> stream);
  void
  serializeNode(SPtr<DataStream> stream, ModelNode* node,
                const UnorderedMap<SPtr<Mesh>, uint32>& meshToIndexMap);
  void
  serializeUniqueMeshes(SPtr<DataStream> stream);
  void
  serializeMesh(SPtr<DataStream> stream, SPtr<Mesh> mesh);
  void
  serializeVertexLayout(SPtr<DataStream> stream, const VertexLayout& layout);

  // Deserialization helper methods
  bool
  deserializeNodeTree(SPtr<DataStream> stream, const Vector<SPtr<Mesh>>& uniqueMeshes);
  bool
  deserializeNode(SPtr<DataStream> stream, ModelNode* parent,
                  const Vector<SPtr<Mesh>>& uniqueMeshes);
  bool
  deserializeUniqueMeshes(SPtr<DataStream> stream, Vector<SPtr<Mesh>>& uniqueMeshes,
                          uint32 expectedMeshCount);
  bool
  deserializeMesh(SPtr<DataStream> stream, SPtr<Mesh> mesh);
  bool
  deserializeVertexLayout(SPtr<DataStream> stream, VertexLayout& layout,
                          uint32 expectedAttributeCount);

  void
  setMeshIndexDataRaw(SPtr<Mesh> mesh, const Vector<uint8>& data, uint32 indexCount,
                      IndexType indexType);

  SPtr<Model> m_model; ///< The model data associated with this asset
}; // class ModelAsset
DECLARE_ASSET_TYPE(ModelAsset);

} // namespace chEngineSDK
