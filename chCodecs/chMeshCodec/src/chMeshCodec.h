/************************************************************************/
/**
 * @file chMeshCodec.h
 * @author AccelMR
 * @date 2025/04/19
 * @brief
 * MeshCodec class for importing model files.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#if USING(CH_CODECS)

#include "chAssetCodec.h"
#include "chTypeTraits.h"
#include "chIAsset.h"

#include "chModel.h"
#include "chModelAsset.h"
#include "chUUID.h"

//Forward declarations from Assimp
struct aiMaterial;
struct aiScene;
struct aiTexture;

namespace chEngineSDK {
class TextureAsset;

/**
 * Exists to turn model files read by Assimp into engine assets. Besides the model it imports
 * the materials of the file and their textures, all in a folder of its own, so the model
 * arrives looking as it did in the tool that made it.
 */
class MeshCodec  : public IAssetCodec {
 public:
  MeshCodec();
  ~MeshCodec() = default;

  UUID
  getCodecType() const override{
    static UUID importType = UUID::createFromName("MeshManagerCodec");
    return importType;
  }

  FORCEINLINE Vector<UUID>
  getSupportedAssetTypes() const override {
    return {AssetTypeTraits<ModelAsset>::getTypeId()};
  }

  NODISCARD const Vector<String>&
  getSupportedExtensions() const override
  {
    return m_extensions;
  }

  SPtr<IAsset>
  importAsset(const Path& filePath, const String& assetName, const Path& assetFolder) override;

 private:
  struct ImportContext
  {
    const aiScene* scene = nullptr;
    // Absolute, so it can be stored as the imported path of everything made from it.
    Path sourceFile;
    Path assetFolder;
    String modelName;
  };

  NODISCARD ModelMaterialSlot
  importMaterial(const ImportContext& context, const aiMaterial& source, uint32 index);

  NODISCARD SPtr<TextureAsset>
  importBaseColorTexture(const ImportContext& context, const aiMaterial& source);

  NODISCARD SPtr<TextureAsset>
  importEmbeddedTexture(const ImportContext& context,
                        const aiTexture& texture,
                        StringView reference);

  Vector<String> m_extensions;
};
DECLARE_ASSET_TYPE(MeshCodec);

} // namespace chEngineSDK
#endif // USING(CH_CODECS)
