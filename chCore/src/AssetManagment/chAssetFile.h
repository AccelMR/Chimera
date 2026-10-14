/************************************************************************/
/**
 * @file chAssetFile.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  The common start of every asset file: header, identity and referenced assets.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chUUID.h"

namespace chEngineSDK {

/**
 * Written first in every asset file, so a file from another program or from an older engine
 * is rejected before anything else is read.
 */
struct AssetFileHeader
{
  static constexpr uint32 MAGIC = 0x53414843; // "CHAS" as the bytes appear in the file
  // Version 1 also stored the name, the folder and the editor data, which now come from
  // where the file is and from its .meta file.
  static constexpr uint32 FORMAT_VERSION = 2;
  static constexpr uint32 OLDEST_READABLE_VERSION = 1;

  uint32 magic = MAGIC;
  uint32 formatVersion = FORMAT_VERSION;
};
static_assert(sizeof(AssetFileHeader) == 8, "AssetFileHeader must have no padding");

/**
 * What the start of an asset file says about the asset.
 */
struct AssetFileInfo
{
  UUID uuid = UUID::null();
  UUID assetType = UUID::null();
  uint32 formatVersion = AssetFileHeader::FORMAT_VERSION;

  // Only filled from files of format version 1, which kept the editor data inside, so the
  // editor can move it into a .meta file.
  uint64 legacyCreationTime = 0;
  String legacyEngineVersion;
  String legacyImportedPath;
};

/**
 * Exists so every reader and writer of asset files agrees on how they start: the header, the
 * asset's UUID and type, then the UUIDs of the assets it references. The data of each asset
 * type follows. It also writes and reads strings with their length, for the asset types.
 */
class CH_CORE_EXPORT AssetFile
{
 public:
  static void
  writeStart(DataStream& stream,
             const UUID& uuid,
             const UUID& assetType,
             Span<const UUID> references);

  /**
   * Reads the header and the asset's identity. Fails on a wrong magic number or a format
   * version it cannot read.
   */
  NODISCARD static bool
  readInfo(DataStream& stream, AssetFileInfo& outInfo);

  /**
   * Call right after readInfo.
   */
  NODISCARD static bool
  readReferences(DataStream& stream, Vector<UUID>& outReferences);

  static void
  writeString(DataStream& stream, StringView text);

  /**
   * Fails when the stored length is above maxLength, which only a broken file has.
   */
  NODISCARD static bool
  readString(DataStream& stream, String& outText, uint32 maxLength);

  // Above any real asset; only a broken file asks for more.
  static constexpr uint32 MAX_REFERENCES = 65536;
  // The metadata block of format version 1, which readInfo skips over.
  static constexpr SIZE_T LEGACY_METADATA_SIZE = 664;
};

} // namespace chEngineSDK
