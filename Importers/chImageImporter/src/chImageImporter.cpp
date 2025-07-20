/************************************************************************/
/**
 * @file chImageImporter.cpp
 * @author AccelMR
 * @date 2025/07/20
 * @brief
 */
/************************************************************************/

#include "chImageImporter.h"

#include "chAssetManager.h"
#include "chFileSystem.h"
#include "chIGraphicsAPI.h"
#include "chLogger.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace chEngineSDK{

CH_LOG_DECLARE_STATIC(ImageImporterLog, All);

namespace ImageImpoterHelpers{
Vector<uint8>
loadImage(const Path& path, int32* width, int32* height, int32* channels) {
  CH_ASSERT(FileSystem::isFile(path));

  Vector<uint8> imageData;
  uint8* data = stbi_load(path.toString().c_str(), width, height, channels, STBI_rgb_alpha);
  if (data) {
    imageData.resize((*width) * (*height) * (static_cast<SIZE_T>(STBI_rgb_alpha)));
    memcpy(imageData.data(), data, imageData.size());
    stbi_image_free(data);
  }
  return imageData;
}
} // namespace ImageImpoterHelpers


/*
*/
Vector<String>
ImageImporter::getSupportedExtensions() const {
  // Add more supported image formats as needed
  return {"png", "jpg", "jpeg", "bmp", "tga", "hdr", "exr"};
}

/*
*/
SPtr<IAsset>
ImageImporter::importAsset(const Path& filePath, const String& assetName) {
  CH_ASSERT(FileSystem::isFile(filePath) && "File does not exist");
  CH_ASSERT(IGraphicsAPI::isStarted() && "Graphics API is not initialized");

  int32 width = 0;
  int32 height = 0;
  int32 channels = 0;
  Vector<uint8> imageData = ImageImpoterHelpers::loadImage(filePath, &width, &height, &channels);

  if (imageData.empty()) {
    CH_LOG_ERROR(ImageImporterLog, "Failed to load image from path: {0}", filePath.toString());
    return nullptr;
  }

  AssetMetadata metadata;
  metadata.uuid = UUID::createRandom();
  metadata.assetType = AssetTypeTraits<TextureAsset>::getTypeId();
  metadata.creationTime = std::chrono::system_clock::now().time_since_epoch().count();
  chString::copyANSI(metadata.typeName, AssetTypeTraits<TextureAsset>::getTypeName());
  chString::copyANSI(metadata.engineVersion, CH_ENGINE_VERSION_STRING);
  chString::copyToANSI(metadata.name, assetName);

  const Path importedPath = FileSystem::absolutePath(Path(filePath));
  chString::copyToANSI(metadata.importedPath, importedPath.toString());
  chString::copyToANSI(metadata.assetPath, EnginePaths::getGameAssetDirectory().toString());

  SPtr<TextureAsset> textureAsset = chMakeShared<TextureAsset>(metadata, imageData, width, height);

  if (!textureAsset->save()) {
    CH_LOG_ERROR(ImageImporterLog, "Failed to save texture asset: " + assetName);
    return nullptr;
  }

  CH_LOG_INFO(ImageImporterLog, "Imported image asset: {0} from {1}", assetName, filePath.toString());
  registerNewAsset(textureAsset);
  return std::static_pointer_cast<IAsset>(textureAsset);
}

/*
*/
bool
ImageImporter::canImport(const String&) const {
  // Check if the extension is supported
  return true;
}

} // namespace chEngineSDK
