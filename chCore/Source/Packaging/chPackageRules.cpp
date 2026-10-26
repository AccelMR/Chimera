/************************************************************************/
/**
 * @file chPackageRules.cpp
 * @author AccelMR
 * @date 2026/10/26
 * @brief
 *  What a root of content asks to put into a package.
 */
/************************************************************************/
#include "chPackageRules.h"

#if USING(CH_EDITOR)
#include "chAlgorithm.h"
#include "chConfigFile.h"
#include "chFileSystem.h"
#include "chIAsset.h"
#include "chStringUtils.h"

namespace chEngineSDK {
namespace {
constexpr StringView kSection = "Package";
constexpr StringView kScenesKey = "Scenes";
constexpr StringView kAlwaysIncludeKey = "AlwaysInclude";

/*
 * A comma list; spaces around each entry are dropped, so hand-written files read the same.
 */
Vector<String>
readList(const ConfigFile& file, StringView key)
{
  Vector<String> entries;
  if (const String* value = file.getValue(kSection, key)) {
    for (const String& entry : StringUtils::splitString(*value, ',')) {
      String trimmed = StringUtils::trim(entry);
      if (!trimmed.empty()) {
        entries.push_back(std::move(trimmed));
      }
    }
  }
  return entries;
}
} // namespace

/*
 */
PackageRules
PackageRules::load(const Path& file)
{
  PackageRules rules;
  ConfigFile config;
  if (FileSystem::exists(file) && config.load(file)) {
    rules.scenes = readList(config, kScenesKey);
    rules.alwaysInclude = readList(config, kAlwaysIncludeKey);
  }
  return rules;
}

/*
 */
bool
PackageRules::save(const Path& file) const
{
  ConfigFile config;
  if (FileSystem::exists(file)) {
    static_cast<void>(config.load(file));
  }
  const PackageRules saved = load(file);
  if (saved.scenes == scenes && saved.alwaysInclude == alwaysInclude) {
    return true;
  }
  config.setValue(kSection, kScenesKey, StringUtils::join(scenes, ","));
  config.setValue(kSection, kAlwaysIncludeKey, StringUtils::join(alwaysInclude, ","));
  return config.save(file);
}

/*
 */
bool
PackageRules::matches(StringView entry, StringView assetPath)
{
  while (entry.size() > 1 && entry.back() == '/') {
    entry.remove_suffix(1);
  }
  if (entry.empty() || !assetPath.starts_with(entry)) {
    return false;
  }
  return assetPath.size() == entry.size() || assetPath[entry.size()] == '/';
}

/*
 */
String
PackageRules::getAssetPath(const IAsset& asset)
{
  String path(asset.getAssetPath());
  path += '/';
  path += asset.getName();
  return path;
}

/*
 */
void
PackageRules::collectAlwaysIncluded(Span<const SPtr<IAsset>> assets,
                                    Vector<UUID>& outRoots,
                                    Vector<String>* outUnmatched) const
{
  for (const String& entry : alwaysInclude) {
    bool bMatched = false;
    for (const SPtr<IAsset>& asset : assets) {
      if (!matches(entry, getAssetPath(*asset))) {
        continue;
      }
      bMatched = true;
      if (!Algorithm::contains(outRoots, asset->getUUID())) {
        outRoots.push_back(asset->getUUID());
      }
    }
    if (!bMatched && outUnmatched) {
      outUnmatched->push_back(entry);
    }
  }
}

} // namespace chEngineSDK
#endif // USING(CH_EDITOR)
