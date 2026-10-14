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
#include "chImportSettings.h"
#include "chPath.h"
#include "chStringUtils.h"

namespace chEngineSDK {

class IAsset;

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
   * sourceFile and sourcePart name where the data came from in the .meta file, so a later
   * import can find the asset. Codecs that cannot read memory return null.
   */
  virtual SPtr<IAsset>
  importAssetFromMemory(Span<const uint8> data,
                        const String& assetName,
                        const Path& assetFolder,
                        const Path& sourceFile,
                        StringView sourcePart,
                        const ImportSettings& settings)
  {
    CH_PARAMETER_UNUSED(data);
    CH_PARAMETER_UNUSED(assetName);
    CH_PARAMETER_UNUSED(assetFolder);
    CH_PARAMETER_UNUSED(sourceFile);
    CH_PARAMETER_UNUSED(sourcePart);
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
   * Records where the asset came from and how (its .meta file), saves it and adds it to the
   * AssetManager.
   */
  NODISCARD bool
  saveAndRegister(const SPtr<IAsset>& asset,
                  const Path& sourceFile,
                  StringView sourcePart,
                  const ImportSettings& settings)
  {
    CH_ASSERT(asset && "Asset cannot be null");
    asset->m_editorData = AssetMeta::makeNew(sourceFile, sourcePart, settings);
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
  makeMetadata(StringView assetName, const Path& assetFolder);

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
IAssetCodec::makeMetadata(StringView assetName, const Path& assetFolder)
{
  AssetMetadata metadata;
  metadata.uuid = UUID::createRandom();
  metadata.assetType = AssetTypeTraits<AssetType>::getTypeId();
  StringUtils::copyToBuffer(metadata.name,
                            AssetManager::instance().makeUniqueAssetName(assetFolder, assetName));
  StringUtils::copyToBuffer(metadata.assetPath, assetFolder.toString());
  return metadata;
}

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
