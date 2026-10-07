/************************************************************************/
/**
 * @file chAssetFile.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  The common start of every asset file: header, metadata and referenced assets.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"

namespace chEngineSDK {

/**
 * Written first in every asset file, so a file from another program or from an older engine
 * is rejected before its metadata is read.
 */
struct AssetFileHeader
{
  static constexpr uint32 MAGIC = 0x53414843; // "CHAS" as the bytes appear in the file
  static constexpr uint32 FORMAT_VERSION = 1;

  uint32 magic = MAGIC;
  uint32 formatVersion = FORMAT_VERSION;
};
static_assert(sizeof(AssetFileHeader) == 8, "AssetFileHeader must have no padding");

/**
 * Exists so every reader and writer of asset files agrees on how they start: the header, the
 * metadata, then the UUIDs of the assets this one references. The data of each asset type
 * follows. It also writes and reads strings with their length, for the asset types.
 */
class CH_CORE_EXPORT AssetFile
{
 public:
  static void
  writeStart(DataStream& stream,
             const AssetMetadata& metadata,
             Span<const UUID> references);

  /**
   * Reads the header and the metadata. Fails on a wrong magic number or format version.
   */
  NODISCARD static bool
  readMetadata(DataStream& stream, AssetMetadata& outMetadata);

  /**
   * Call right after readMetadata.
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

  static constexpr SIZE_T METADATA_OFFSET = sizeof(AssetFileHeader);
  // Above any real asset; only a broken file asks for more.
  static constexpr uint32 MAX_REFERENCES = 65536;
};

} // namespace chEngineSDK
