/************************************************************************/
/**
 * @file chPackageRules.h
 * @author AccelMR
 * @date 2026/10/26
 * @brief
 *  What a root of content asks to put into a package.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#if USING(CH_EDITOR)
#include "chPath.h"
#include "chUUID.h"

namespace chEngineSDK {
class IAsset;

/**
 * Exists so what goes into a package is written in one place for each root of content (the
 * project today, every plugin with content later), never in scripts: the scenes to package
 * besides the startup one, and the folders or assets always copied, for what code only names
 * by text (Scene.instantiate("/Game/Enemies/Orc")). Each root keeps them in its
 * Config/Package.ini, which is never copied into a package.
 */
struct CH_CORE_EXPORT PackageRules
{
  /**
   * A missing file gives empty rules.
   */
  NODISCARD static PackageRules
  load(const Path& file);

  /**
   * Writes only when the file holds other rules, so a file under version control is not
   * touched for nothing; other sections of the file are kept. False when it could not be
   * written.
   */
  NODISCARD bool
  save(const Path& file) const;

  /**
   * True when assetPath (a virtual path without extension, /Game/Enemies/Orc) is entry or is
   * inside the folder entry names.
   */
  NODISCARD static bool
  matches(StringView entry, StringView assetPath);

  /**
   * The virtual path of an asset without extension, as entries and scenes name it.
   */
  NODISCARD static String
  getAssetPath(const IAsset& asset);

  /**
   * Appends the assets an alwaysInclude entry names, each once, to outRoots, and each entry
   * that names none to outUnmatched (may be null).
   */
  void
  collectAlwaysIncluded(Span<const SPtr<IAsset>> assets,
                        Vector<UUID>& outRoots,
                        Vector<String>* outUnmatched) const;

  // Virtual paths of scenes, besides the startup one.
  Vector<String> scenes;
  // Virtual paths of folders (with everything inside them) or of assets.
  Vector<String> alwaysInclude;

  static constexpr StringView FILE_NAME = "Package.ini";
};

/**
 * A root of content: its mount (/Game) and the file its rules are kept in.
 */
struct PackageContentSource
{
  String mountName;
  Path rulesFile;
};

} // namespace chEngineSDK
#endif // USING(CH_EDITOR)
