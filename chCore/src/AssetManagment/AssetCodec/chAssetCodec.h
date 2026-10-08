/************************************************************************/
/**
 * @file chAssetCodec.h
 * @author AccelMR
 * @date 2025/07/12
 * @brief  Asset codec interface for Chimera Core.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#if USING(CH_EDITOR)
#include "chUUID.h"
#include "chIAsset.h"
#include "chAssetManager.h"
#include "chPath.h"
#include "chStringUtils.h"

namespace chEngineSDK {

class IAsset;

/**
 * Exists so whoever starts an import (the user, or a codec importing what a file uses) can
 * say how the data is meant to be used. Each codec reads only the fields it understands.
 */
struct ImportSettings
{
  // Textures: color data (base color, emissive) is stored as sRGB, so the GPU turns it into
  // linear values when it samples; normals and masks are data and stay linear.
  bool srgb = true;
  bool generateMips = true;
};

/**
 * Turns an external file (image, mesh...) into an engine asset. Codecs live in plugins
 * and register themselves with AssetCodecManager, which picks one by file extension.
 */
class CH_CORE_EXPORT IAssetCodec {
 public:
  IAssetCodec() = default;
  virtual ~IAssetCodec() = default;

  virtual UUID
  getCodecType() const = 0;

  /**
   * Lowercase, without the dot ("png"). Built once, so calling it does not allocate.
   */
  NODISCARD virtual const Vector<String>&
  getSupportedExtensions() const = 0;

  /**
   * Saves the new asset (and anything it needs) in assetFolder, a virtual path under /Game.
   * The name gets a suffix when the folder already has an asset with it.
   */
  virtual SPtr<IAsset>
  importAsset(const Path& filePath,
              const String& assetName,
              const Path& assetFolder,
              const ImportSettings& settings) = 0;

  /**
   * Same as importAsset for a file already in memory (a texture inside a model file).
   * importedPath names the source in the metadata, so a later import can find the asset.
   * Codecs that cannot read memory return null.
   */
  virtual SPtr<IAsset>
  importAssetFromMemory(Span<const uint8> data,
                        const String& assetName,
                        const Path& assetFolder,
                        StringView importedPath,
                        const ImportSettings& settings)
  {
    CH_PARAMETER_UNUSED(data);
    CH_PARAMETER_UNUSED(assetName);
    CH_PARAMETER_UNUSED(assetFolder);
    CH_PARAMETER_UNUSED(importedPath);
    CH_PARAMETER_UNUSED(settings);
    return nullptr;
  }

  /**
   * Accepts the extension with or without the dot, in any case (".PNG").
   */
  NODISCARD bool
  canImport(StringView extension) const
  {
    extension = withoutDot(extension);
    for (const String& supported : getSupportedExtensions()) {
      if (StringUtils::equalsIgnoreCase(supported, extension)) {
        return true;
      }
    }
    return false;
  }

  virtual Vector<UUID>
  getSupportedAssetTypes() const = 0;

  void
  registerNewAsset(const SPtr<IAsset>& asset) {
    CH_ASSERT(asset && "Asset cannot be null");
    AssetManager::instance().registerNewAsset(asset);
  }

  /**
   * Saves the asset and adds it to the AssetManager.
   */
  NODISCARD bool
  saveAndRegister(const SPtr<IAsset>& asset)
  {
    if (!AssetManager::instance().saveAsset(asset)) {
      return false;
    }
    registerNewAsset(asset);
    return true;
  }

  /**
   * Metadata of a new asset of that type, with a fresh UUID and a name no other asset in
   * the folder has.
   */
  template <typename AssetType>
  NODISCARD static AssetMetadata
  makeMetadata(StringView assetName, StringView importedPath, const Path& assetFolder);

  /**
   * The form extensions are stored and looked up in: lowercase, without the dot.
   */
  NODISCARD static String
  normalizeExtension(StringView extension)
  {
    return StringUtils::toLower(String(withoutDot(extension)));
  }

 private:
  NODISCARD static StringView
  withoutDot(StringView extension) noexcept
  {
    if (!extension.empty() && '.' == extension.front()) {
      extension.remove_prefix(1);
    }
    return extension;
  }
};

/*
 */
template <typename AssetType>
AssetMetadata
IAssetCodec::makeMetadata(StringView assetName, StringView importedPath, const Path& assetFolder)
{
  AssetMetadata metadata;
  metadata.uuid = UUID::createRandom();
  metadata.assetType = AssetTypeTraits<AssetType>::getTypeId();
  metadata.creationTime = std::chrono::system_clock::now().time_since_epoch().count();
  StringUtils::copyToBuffer(metadata.typeName, AssetTypeTraits<AssetType>::getTypeName());
  StringUtils::copyToBuffer(metadata.engineVersion, CH_ENGINE_VERSION_STRING);
  StringUtils::copyToBuffer(metadata.name,
                            AssetManager::instance().makeUniqueAssetName(assetFolder, assetName));
  StringUtils::copyToBuffer(metadata.importedPath, importedPath);
  StringUtils::copyToBuffer(metadata.assetPath, assetFolder.toString());
  return metadata;
}

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
