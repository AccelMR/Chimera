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
  // Nothing has shipped yet, so earlier layouts were dropped instead of read: files saved
  // before 2026-10-22 must be imported again.
  static constexpr uint32 FORMAT_VERSION = 1;

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
};

/**
 * Exists so every reader and writer of asset files agrees on how they start: the header, the
 * asset's UUID and type, the UUIDs of the assets it references, then the editor data (text
 * with its length, which the game skips without reading). The data of each asset type
 * follows. It also writes and reads strings with their length, for the asset types.
 */
class CH_CORE_EXPORT AssetFile
{
 public:
  /**
   * editorData is empty in builds without the editor and in packaged games.
   */
  static void
  writeStart(DataStream& stream,
             const UUID& uuid,
             const UUID& assetType,
             Span<const UUID> references,
             StringView editorData = StringView());

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

  /**
   * Call right after readReferences. Reads the editor data into outText, or only moves past
   * it when outText is null.
   */
  NODISCARD static bool
  readEditorData(DataStream& stream, String* outText);

  /**
   * Writes source again into destination (which may be the same file) with editorData
   * instead of its own, keeping the asset's data byte for byte. Package Project passes no
   * editor data.
   */
  NODISCARD static bool
  rewriteWithEditorData(const Path& source, const Path& destination, StringView editorData);

  static void
  writeString(DataStream& stream, StringView text);

  /**
   * Fails when the stored length is above maxLength, which only a broken file has.
   */
  NODISCARD static bool
  readString(DataStream& stream, String& outText, uint32 maxLength);

  /**
   * Reads exactly size bytes; false when the data ends first.
   */
  NODISCARD static bool
  readBytes(DataStream& stream, void* outData, SIZE_T size);

  template<typename T>
  NODISCARD static bool
  readValue(DataStream& stream, T& outValue)
  {
    return readBytes(stream, &outValue, sizeof(T));
  }

  /**
   * Bytes left to read, so a stored size can be checked before anything is allocated for it.
   */
  NODISCARD static SIZE_T
  getBytesLeft(const DataStream& stream);

  // Above any real asset; only a broken file asks for more.
  static constexpr uint32 MAX_REFERENCES = 65536;
  static constexpr uint32 MAX_EDITOR_DATA_SIZE = 1024 * 1024;
};

} // namespace chEngineSDK
