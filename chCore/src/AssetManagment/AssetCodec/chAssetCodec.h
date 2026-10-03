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

  virtual SPtr<IAsset>
  importAsset(const Path& filePath, const String& assetName) = 0;

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

  template <typename AssetType = IAsset>
  FORCEINLINE SPtr<AssetType>
  importAsset(const Path& filePath, const String& assetName) {
    return std::static_pointer_cast<AssetType>(importAsset(filePath, assetName));
  }

  void
  registerNewAsset(const SPtr<IAsset>& asset) {
    CH_ASSERT(asset && "Asset cannot be null");
    AssetManager::instance().registerNewAsset(asset);
  }

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

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
