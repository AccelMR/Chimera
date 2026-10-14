/************************************************************************/
/**
 * @file chAssetMeta.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Editor data of an asset, kept in a .meta file next to the asset file.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#if USING(CH_EDITOR)
#include "chImportSettings.h"
#include "chPath.h"

namespace chEngineSDK {
class ConfigFile;

/**
 * Exists so the editor knows when and from what an asset was made while the game never reads
 * it: it is saved in the asset's .meta file, not in the asset file.
 */
struct AssetEditorData
{
  // Seconds since 1970 (UTC); 0 when unknown.
  uint64 creationTime = 0;
  String engineVersion;
  // Absolute path of the source file; empty for assets made in the editor.
  Path sourceFile;
  // The piece of the source file the asset came from (a material or an embedded texture of
  // a model), empty when it is the whole file.
  String sourcePart;
  ImportSettings importSettings;
};

/**
 * Exists so the .meta file is written and read in one place. It is a text config file, so it
 * reads well in a diff. The source path is stored relative to the project, so a project moved
 * together with its source files keeps it.
 */
class CH_CORE_EXPORT AssetMeta
{
 public:
  /**
   * "<folder>/<Name>.chAss.meta" for "<folder>/<Name>.chAss".
   */
  NODISCARD static Path
  getPath(const Path& assetFile);

  /**
   * The source path is written relative to baseFolder when it can be.
   */
  static void
  write(const AssetEditorData& data, const Path& baseFolder, ConfigFile& outFile);

  /**
   * Missing or broken values keep what outData had.
   */
  static void
  read(const ConfigFile& file, const Path& baseFolder, AssetEditorData& outData);

  NODISCARD static bool
  save(const Path& assetFile, const AssetEditorData& data);

  /**
   * False, with outData untouched, when the asset has no .meta file.
   */
  NODISCARD static bool
  load(const Path& assetFile, AssetEditorData& outData);

  /**
   * Editor data of an asset made now from that source.
   */
  NODISCARD static AssetEditorData
  makeNew(const Path& sourceFile, StringView sourcePart, const ImportSettings& settings);

  NODISCARD static uint64
  getCurrentTime();

  NODISCARD static StringView
  getExpandModeName(ModelExpandMode mode) noexcept;

  static constexpr StringView EXTENSION = ".meta";
};

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
