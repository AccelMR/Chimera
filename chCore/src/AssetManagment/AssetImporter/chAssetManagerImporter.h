#pragma once

#include "chPrerequisitesCore.h"
#include "chLogDeclaration.h"
CH_LOG_DECLARE_EXTERN(AssetImporterSystem);

#if USING(CH_EDITOR)

#include "chAssetImporter.h"
#include "chAssetImporterRegistry.h"
#include "chEventSystem.h"
#include "chModule.h"

namespace chEngineSDK {

class CH_CORE_EXPORT AssetManagerImporter : public Module<AssetManagerImporter> {
 public:

  void
  initialize();

  /**
   * @brief Imports an asset from the specified import path to the asset path.
   * @param importPath The path to the asset to be imported.
   * @param assetPath The destination path for the imported asset.
   * @return True if the import was successful, false otherwise.
   */
  bool
  importAsset(const Path& importPath, const Path& assetPath);

  template <typename AssetImporterType = IAssetImporter>
  SPtr<AssetImporterType>
  getImporter() const {
    CH_ASSERT(m_importerRegistry && "AssetImporterRegistry must be initialized before accessing importers.");
    return m_importerRegistry->getImporter<AssetImporterType>();
  }

 private:
  SPtr<AssetImporterRegistry> m_importerRegistry;
  Event<bool(const SPtr<AssetImporterRegistry>&)> m_onRegisterImporter;

}; // class AssetManagerImporter
} // namespace chEngineSDK
#else // USING(CH_EDITOR)
#endif // USING(CH_EDITOR)
