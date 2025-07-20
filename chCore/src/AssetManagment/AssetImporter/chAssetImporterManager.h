#pragma once

#include "chLogDeclaration.h"
#include "chPrerequisitesCore.h"
CH_LOG_DECLARE_EXTERN(AssetImporterSystem);

#if USING(CH_IMPORTERS)

#include "chAssetImporter.h"
#include "chAssetImporterRegistry.h"
#include "chEventSystem.h"
#include "chModule.h"

namespace chEngineSDK {

class CH_CORE_EXPORT AssetImporterManager : public Module<AssetImporterManager>
{
 public:
  void
  initialize();

  /**
   * @brief Imports an asset from the specified import path to the asset path.
   * @param importPath The path to the asset to be imported.
   * @param assetPath The destination path for the imported asset.
   * @return True if the import was successful, false otherwise.
   */
  SPtr<IAsset>
  importAsset(const Path& importPath, const Path& assetPath);

  template <typename AssetType = IAsset>
  FORCEINLINE SPtr<AssetType>
  importAsset(const Path& importPath, const String& assetName) {
    return std::static_pointer_cast<AssetType>(importAsset(importPath, assetName));
  }

  template <typename AssetImporterType = IAssetImporter>
  FORCEINLINE SPtr<AssetImporterType>
  getImporter() const {
    CH_ASSERT(m_importerRegistry &&
              "AssetImporterRegistry must be initialized before accessing importers.");
    return m_importerRegistry->getImporter<AssetImporterType>();
  }

  FORCEINLINE Vector<SPtr<IAssetImporter>>
  getAllImporters() const {
    CH_ASSERT(m_importerRegistry &&
              "AssetImporterRegistry must be initialized before accessing importers.");
    return m_importerRegistry->getAllImporters();
  }

  Vector<String>
  getSupportedAllExtensions() const;

  SPtr<IAssetImporter>
  FORCEINLINE getImporterForExtension(const String& extension) const {
    CH_ASSERT(m_importerRegistry &&
              "AssetImporterRegistry must be initialized before accessing importers.");
    return m_importerRegistry->getImporterForExtension(extension);
  }

  SPtr<IAssetImporter>
  FORCEINLINE getImporterForAssetType(const UUID& assetType) const {
    CH_ASSERT(m_importerRegistry &&
              "AssetImporterRegistry must be initialized before accessing importers.");
    return m_importerRegistry->getImporterForAssetType(assetType);
  }

  template <typename AssetType = IAsset>
  SPtr<IAssetImporter>
  FORCEINLINE getImporterForAssetType() const {
    CH_ASSERT(m_importerRegistry &&
              "AssetImporterRegistry must be initialized before accessing importers.");
    return m_importerRegistry->getImporterForAssetType<AssetType>();
  }

  template <typename AssetImporterType = IAssetImporter>
  FORCEINLINE void
  registerImporter() {
    CH_ASSERT(m_importerRegistry &&
              "AssetImporterRegistry must be initialized before registering importers.");
    m_importerRegistry->registerImporter<AssetImporterType>();
  }

 private:
  UniquePtr<AssetImporterRegistry> m_importerRegistry;
}; // class AssetImporterManager
} // namespace chEngineSDK
#endif  // USING(CH_EDITOR)
