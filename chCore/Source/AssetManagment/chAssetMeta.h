/************************************************************************/
/**
 * @file chAssetMeta.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Editor data of an asset, kept as text inside the asset file.
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
 * it: it is saved as text in the asset file, which the game skips and packages drop.
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
 * Exists so the editor data is written and read in one place. It is config file text, so it
 * can still be read when debugging a file. The source path is stored relative to the project,
 * so a project moved together with its source files keeps it.
 */
class CH_CORE_EXPORT AssetMeta
{
 public:
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

  /**
   * Editor data of an asset made now from that source.
   */
  NODISCARD static AssetEditorData
  makeNew(const Path& sourceFile, StringView sourcePart, const ImportSettings& settings);

  NODISCARD static uint64
  getCurrentTime();

  NODISCARD static StringView
  getExpandModeName(ModelExpandMode mode) noexcept;
};

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
