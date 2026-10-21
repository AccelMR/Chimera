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

namespace chEngineSDK {

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

  /**
   * The worker part reads the file, converts the meshes and decodes the textures; the main
   * thread part makes the folder, the materials and the assets, reusing what an earlier
   * import of the same file made.
   */
  NODISCARD UniquePtr<IImportTask>
  createImportTask(const Path& filePath,
                   const String& assetName,
                   const ImportSettings& settings) override;

 private:
  Vector<String> m_extensions;
};
DECLARE_ASSET_TYPE(MeshCodec);

} // namespace chEngineSDK
#endif // USING(CH_CODECS)
