/************************************************************************/
/**
 * @file chAssetFile.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  The common start of every asset file: header, identity and referenced assets.
 */
/************************************************************************/
#include "chAssetFile.h"

#include "chAssetManager.h"
#include "chFileStream.h"
#include "chLogger.h"

namespace chEngineSDK {
namespace {
// The metadata block of format version 1, as it was written with a raw copy.
struct LegacyMetadataV1
{
  UUID uuid;
  UUID assetType;
  uint64 creationTime;
  ANSICHAR typeName[32];
  ANSICHAR engineVersion[16];
  ANSICHAR name[64];
  ANSICHAR importedPath[256];
  ANSICHAR assetPath[256];
};
static_assert(sizeof(LegacyMetadataV1) == AssetFile::LEGACY_METADATA_SIZE,
              "LegacyMetadataV1 must match the files of format version 1");

/*
 * The text before the first '\0', or the whole field when a broken file has none.
 */
template <SIZE_T Size>
StringView
fieldText(const ANSICHAR (&field)[Size]) noexcept
{
  SIZE_T length = 0;
  while (length < Size && field[length] != '\0') {
    ++length;
  }
  return StringView(field, length);
}
} // namespace

/*
 */
void
AssetFile::writeStart(DataStream& stream,
                      const UUID& uuid,
                      const UUID& assetType,
                      Span<const UUID> references)
{
  const AssetFileHeader header;
  stream.write(&header, sizeof(header));
  stream.write(&uuid, sizeof(uuid));
  stream.write(&assetType, sizeof(assetType));

  const uint32 referenceCount = static_cast<uint32>(references.size());
  stream.write(&referenceCount, sizeof(referenceCount));
  if (referenceCount > 0) {
    stream.write(references.data(), references.size_bytes());
  }
}

/*
 */
bool
AssetFile::readInfo(DataStream& stream, AssetFileInfo& outInfo)
{
  AssetFileHeader header;
  if (stream.read(&header, sizeof(header)) != sizeof(header) ||
      header.magic != AssetFileHeader::MAGIC) {
    CH_LOG_ERROR(AssetSystem,
                 "Not an asset file, or saved before format version 1; import it again.");
    return false;
  }
  if (header.formatVersion < AssetFileHeader::OLDEST_READABLE_VERSION ||
      header.formatVersion > AssetFileHeader::FORMAT_VERSION) {
    CH_LOG_ERROR(AssetSystem, "Unsupported asset format version {0} (expected {1} to {2}).",
                 header.formatVersion, AssetFileHeader::OLDEST_READABLE_VERSION,
                 AssetFileHeader::FORMAT_VERSION);
    return false;
  }
  outInfo.formatVersion = header.formatVersion;

  if (header.formatVersion == 1) {
    LegacyMetadataV1 legacy;
    if (stream.read(&legacy, sizeof(legacy)) != sizeof(legacy)) {
      CH_LOG_ERROR(AssetSystem, "Asset file ends inside its metadata.");
      return false;
    }
    outInfo.uuid = legacy.uuid;
    outInfo.assetType = legacy.assetType;
    outInfo.legacyCreationTime = legacy.creationTime;
    outInfo.legacyEngineVersion = fieldText(legacy.engineVersion);
    outInfo.legacyImportedPath = fieldText(legacy.importedPath);
    return true;
  }

  if (stream.read(&outInfo.uuid, sizeof(UUID)) != sizeof(UUID) ||
      stream.read(&outInfo.assetType, sizeof(UUID)) != sizeof(UUID)) {
    CH_LOG_ERROR(AssetSystem, "Asset file ends inside its header.");
    return false;
  }
  return true;
}

/*
 */
bool
AssetFile::readReferences(DataStream& stream, Vector<UUID>& outReferences)
{
  outReferences.clear();

  uint32 referenceCount = 0;
  if (stream.read(&referenceCount, sizeof(referenceCount)) != sizeof(referenceCount) ||
      referenceCount > MAX_REFERENCES) {
    CH_LOG_ERROR(AssetSystem, "Asset file has a broken reference list.");
    return false;
  }

  outReferences.resize(referenceCount);
  const SIZE_T byteCount = referenceCount * sizeof(UUID);
  if (byteCount > 0 && stream.read(outReferences.data(), byteCount) != byteCount) {
    CH_LOG_ERROR(AssetSystem, "Asset file ends inside its reference list.");
    outReferences.clear();
    return false;
  }
  return true;
}

/*
 */
void
AssetFile::writeString(DataStream& stream, StringView text)
{
  const uint32 length = static_cast<uint32>(text.size());
  stream.write(&length, sizeof(length));
  if (length > 0) {
    stream.write(text.data(), length);
  }
}

/*
 */
bool
AssetFile::readString(DataStream& stream, String& outText, uint32 maxLength)
{
  uint32 length = 0;
  if (stream.read(&length, sizeof(length)) != sizeof(length) || length > maxLength) {
    return false;
  }

  outText.resize(length);
  return length == 0 || stream.read(outText.data(), length) == length;
}

} // namespace chEngineSDK
