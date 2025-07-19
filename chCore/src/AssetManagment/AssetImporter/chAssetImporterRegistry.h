/************************************************************************/
/**
 * @file chAssetImporterRegistry.h
 * @author AccelMR
 * @date 2025/07/12
 * @details  Asset importer registry for Chimera Core.
 */
/***************************************************** *******************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAssetImporter.h"
#include "chAssetTypeTraits.h"

#include "chUUID.h"

namespace chEngineSDK {
class IAssetImporter;

struct RegisterResponse {
  bool success;
  String message;
};

class AssetImporterRegistry
{
 public:

  template <typename AssetImporterType = IAssetImporter>
  void
  registerImporter() {
    SPtr<AssetImporterType> importer = chMakeShared<AssetImporterType>();

    for (const String& ext : importer->getSupportedExtensions()) {
      m_extensionToImporter[ext] = importer;
    }

    m_typeToImporter[AssetTypeTraits<AssetImporterType>::getTypeId()] = importer;
  }

  /*
  */
  template <typename AssetImporterType = IAssetImporter>
  SPtr<AssetImporterType>
  getImporter() const {
    auto it = m_typeToImporter.find(AssetTypeTraits<AssetImporterType>::getTypeId());
    if (it != m_typeToImporter.end()) {
      return std::static_pointer_cast<AssetImporterType>(it->second);
    }
    return nullptr;
  }

  SPtr<IAssetImporter>
  getImporterForExtension(const String& extension) const {
    auto it = m_extensionToImporter.find(extension);
    return (it != m_extensionToImporter.end()) ? it->second : nullptr;
  }

  SPtr<IAssetImporter>
  getImporterForAssetType(const UUID& assetType) const {
    auto it = m_typeToImporter.find(assetType);
    return (it != m_typeToImporter.end()) ? it->second : nullptr;
  }

  template <typename AssetType = IAsset>
  SPtr<IAssetImporter>
  getImporterForAssetType() const {
    return getImporterForAssetType(AssetTypeTraits<AssetType>::getTypeId());
  }

  const Vector<SPtr<IAssetImporter>>
  getAllImporters() const {
    Vector<SPtr<IAssetImporter>> importers;
    importers.reserve(m_typeToImporter.size());

    auto importer_view = m_typeToImporter | std::views::transform([](const auto& pair) {
        return pair.second;
    });

    importers.assign(importer_view.begin(), importer_view.end());
    return importers;
  }

 private:
  UnorderedMap<String, SPtr<IAssetImporter>> m_extensionToImporter;
  UnorderedMap<UUID, SPtr<IAssetImporter>> m_typeToImporter;
};
} // namespace chEngineSDK
