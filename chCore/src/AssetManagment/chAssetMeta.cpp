/************************************************************************/
/**
 * @file chAssetMeta.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Editor data of an asset, kept in a .meta file next to the asset file.
 */
/************************************************************************/
#include "chAssetMeta.h"

#if USING(CH_EDITOR)
#include <charconv>
#include <chrono>

#include "chConfigFile.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chStringUtils.h"

namespace chEngineSDK {
namespace {
constexpr StringView kAssetSection = "Asset";
constexpr StringView kImportSection = "Import";

// Indexed by ModelExpandMode.
constexpr StringView kExpandModeNames[] = {"Single", "PerNode", "PerMesh"};
// The Skeleton value, unless it is the UUID of an existing skeleton.
constexpr StringView kSkeletonAuto = "Auto";
constexpr StringView kSkeletonNew = "New";

/*
 */
StringView
boolText(bool value) noexcept
{
  return value ? "true" : "false";
}

/*
 */
void
readBool(const ConfigFile& file, StringView key, bool& value)
{
  const String* text = file.getValue(kImportSection, key);
  if (!text) {
    return;
  }
  if (StringUtils::equalsIgnoreCase(*text, "true") || *text == "1") {
    value = true;
  }
  else if (StringUtils::equalsIgnoreCase(*text, "false") || *text == "0") {
    value = false;
  }
}
} // namespace

/*
 */
Path
AssetMeta::getPath(const Path& assetFile)
{
  String text = assetFile.toString();
  text += EXTENSION;
  return Path(std::move(text));
}

/*
 */
void
AssetMeta::write(const AssetEditorData& data, const Path& baseFolder, ConfigFile& outFile)
{
  ANSICHAR numberBuffer[StringUtils::MAX_INTEGER_CHARS];
  outFile.setValue(kAssetSection, "Created", StringUtils::toChars(numberBuffer,
                                                                  data.creationTime));
  outFile.setValue(kAssetSection, "EngineVersion", data.engineVersion);

  if (!data.sourceFile.empty()) {
    const Path relative = FileSystem::relativePath(baseFolder, data.sourceFile);
    outFile.setValue(kImportSection, "Source",
                     relative.empty() ? data.sourceFile.toString() : relative.toString());
  }
  if (!data.sourcePart.empty()) {
    outFile.setValue(kImportSection, "Part", data.sourcePart);
  }

  const ImportSettings& settings = data.importSettings;
  outFile.setValue(kImportSection, "SRGB", boolText(settings.srgb));
  outFile.setValue(kImportSection, "GenerateMips", boolText(settings.generateMips));
  outFile.setValue(kImportSection, "ImportModel", boolText(settings.importModel));
  outFile.setValue(kImportSection, "ExpandMode", getExpandModeName(settings.expandMode));
  outFile.setValue(kImportSection, "SplitParts", boolText(settings.splitParts));
  outFile.setValue(kImportSection, "AutoAlphaMask", boolText(settings.autoAlphaMask));
  outFile.setValue(kImportSection, "ImportAnimations", boolText(settings.importAnimations));
  ANSICHAR floatBuffer[StringUtils::MAX_FLOAT_CHARS];
  outFile.setValue(kImportSection, "AnimationSampleRate",
                   StringUtils::toChars(floatBuffer, settings.animationSampleRate));
  if (settings.skeletonMode == ImportSkeletonMode::Existing && !settings.skeleton.isNull()) {
    outFile.setValue(kImportSection, "Skeleton", settings.skeleton.toString());
  }
  else {
    outFile.setValue(kImportSection, "Skeleton",
                     settings.skeletonMode == ImportSkeletonMode::CreateNew ? kSkeletonNew
                                                                            : kSkeletonAuto);
  }
}

/*
 */
void
AssetMeta::read(const ConfigFile& file, const Path& baseFolder, AssetEditorData& outData)
{
  if (const String* text = file.getValue(kAssetSection, "Created")) {
    uint64 value = 0;
    const ANSICHAR* const end = text->data() + text->size();
    const std::from_chars_result result = std::from_chars(text->data(), end, value);
    if (result.ec == std::errc() && result.ptr == end) {
      outData.creationTime = value;
    }
  }
  if (const String* text = file.getValue(kAssetSection, "EngineVersion")) {
    outData.engineVersion = *text;
  }

  if (const String* text = file.getValue(kImportSection, "Source")) {
    const Path source(*text);
    outData.sourceFile = FileSystem::absolutePath(source.isRelative() ? baseFolder.join(source)
                                                                      : source);
  }
  if (const String* text = file.getValue(kImportSection, "Part")) {
    outData.sourcePart = *text;
  }

  ImportSettings& settings = outData.importSettings;
  readBool(file, "SRGB", settings.srgb);
  readBool(file, "GenerateMips", settings.generateMips);
  readBool(file, "ImportModel", settings.importModel);
  readBool(file, "SplitParts", settings.splitParts);
  readBool(file, "AutoAlphaMask", settings.autoAlphaMask);
  readBool(file, "ImportAnimations", settings.importAnimations);
  if (const String* text = file.getValue(kImportSection, "AnimationSampleRate")) {
    float value = 0.0f;
    const ANSICHAR* const end = text->data() + text->size();
    const std::from_chars_result result = std::from_chars(text->data(), end, value);
    if (result.ec == std::errc() && result.ptr == end && value >= 1.0f) {
      settings.animationSampleRate = value;
    }
  }
  if (const String* text = file.getValue(kImportSection, "ExpandMode")) {
    for (uint32 i = 0; i < std::size(kExpandModeNames); ++i) {
      if (StringUtils::equalsIgnoreCase(*text, kExpandModeNames[i])) {
        settings.expandMode = static_cast<ModelExpandMode>(i);
      }
    }
  }
  if (const String* text = file.getValue(kImportSection, "Skeleton")) {
    if (StringUtils::equalsIgnoreCase(*text, kSkeletonNew)) {
      settings.skeletonMode = ImportSkeletonMode::CreateNew;
    }
    else if (UUID::fromString(*text, settings.skeleton)) {
      settings.skeletonMode = ImportSkeletonMode::Existing;
    }
  }
}

/*
 */
bool
AssetMeta::save(const Path& assetFile, const AssetEditorData& data)
{
  ConfigFile file;
  write(data, EnginePaths::getProjectDirectory(), file);
  return file.save(getPath(assetFile));
}

/*
 */
AssetEditorData
AssetMeta::makeNew(const Path& sourceFile, StringView sourcePart, const ImportSettings& settings)
{
  return AssetEditorData{.creationTime = getCurrentTime(),
                         .engineVersion = CH_ENGINE_VERSION_STRING,
                         .sourceFile = sourceFile,
                         .sourcePart = String(sourcePart),
                         .importSettings = settings};
}

/*
 */
uint64
AssetMeta::getCurrentTime()
{
  // Since C++20 the system clock counts from 1970 in UTC on every platform.
  const auto sinceEpoch = std::chrono::system_clock::now().time_since_epoch();
  return static_cast<uint64>(
      std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch).count());
}

/*
 */
StringView
AssetMeta::getExpandModeName(ModelExpandMode mode) noexcept
{
  const uint32 index = static_cast<uint32>(mode);
  return index < std::size(kExpandModeNames) ? kExpandModeNames[index] : kExpandModeNames[0];
}

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
