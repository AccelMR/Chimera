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
#include "chFileSystem.h"
#include "chPath.h"
#include "chLogger.h"

namespace chEngineSDK {
/*
 */
void
AssetFile::writeStart(DataStream& stream,
                      const UUID& uuid,
                      const UUID& assetType,
                      Span<const UUID> references,
                      StringView editorData)
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
  writeString(stream, editorData);
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
                 "Not an asset file; import it again.");
    return false;
  }
  if (header.formatVersion != AssetFileHeader::FORMAT_VERSION) {
    CH_LOG_ERROR(AssetSystem,
                 "Unsupported asset format version {0} (expected {1}); import it again.",
                 header.formatVersion, AssetFileHeader::FORMAT_VERSION);
    return false;
  }
  outInfo.formatVersion = header.formatVersion;

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
bool
AssetFile::readEditorData(DataStream& stream, String* outText)
{
  if (outText) {
    outText->clear();
  }

  uint32 length = 0;
  if (!readValue(stream, length) || length > MAX_EDITOR_DATA_SIZE ||
      length > getBytesLeft(stream)) {
    CH_LOG_ERROR(AssetSystem, "Asset file has broken editor data.");
    return false;
  }
  if (!outText) {
    stream.seek(stream.tell() + length);
    return true;
  }
  outText->resize(length);
  return readBytes(stream, outText->data(), length);
}

/*
 */
bool
AssetFile::rewriteWithEditorData(const Path& source,
                                 const Path& destination,
                                 StringView editorData)
{
  // Read whole first, so the destination may be the source.
  Vector<uint8> bytes = FileSystem::fastRead(source);
  if (bytes.empty()) {
    CH_LOG_ERROR(AssetSystem, "Could not read {0}", source);
    return false;
  }

  MemoryDataStream input(bytes.data(), bytes.size(), false);
  AssetFileInfo info;
  Vector<UUID> references;
  if (!readInfo(input, info) || !readReferences(input, references) ||
      !readEditorData(input, nullptr)) {
    CH_LOG_ERROR(AssetSystem, "Could not read the asset file {0}", source);
    return false;
  }
  const SIZE_T dataStart = input.tell();

  SPtr<DataStream> output = FileSystem::createAndOpenFile(destination);
  if (!output) {
    CH_LOG_ERROR(AssetSystem, "Could not write {0}", destination);
    return false;
  }
  writeStart(*output, info.uuid, info.assetType, references, editorData);
  const SIZE_T dataSize = bytes.size() - dataStart;
  const bool bWritten =
      dataSize == 0 || output->write(bytes.data() + dataStart, dataSize) == dataSize;
  output->close();
  if (!bWritten) {
    CH_LOG_ERROR(AssetSystem, "Could not write {0}", destination);
  }
  return bWritten;
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

/*
 */
bool
AssetFile::readBytes(DataStream& stream, void* outData, SIZE_T size)
{
  return size == 0 || stream.read(outData, size) == size;
}

/*
 */
SIZE_T
AssetFile::getBytesLeft(const DataStream& stream)
{
  const SIZE_T position = stream.tell();
  return stream.size() > position ? stream.size() - position : 0;
}

} // namespace chEngineSDK
