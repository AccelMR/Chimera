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

 private:
  UnorderedMap<String, SPtr<IAssetImporter>> m_extensionToImporter;
  UnorderedMap<UUID, SPtr<IAssetImporter>> m_typeToImporter;
};
} // namespace chEngineSDK
