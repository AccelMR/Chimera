/************************************************************************/
/**
 * @file chScriptAsset.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  A Lua script of the project.
 */
/************************************************************************/
#include "chScriptAsset.h"

#include "chFileStream.h"
#include "chLogger.h"
#include "chStringUtils.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(ScriptAssetLog, All);

/*
 */
ScriptAsset::ScriptAsset(const AssetMetadata& metadata, String source)
  : IAsset(metadata),
    m_source(std::move(source))
{
  // Its text is already here, so there is nothing to read.
  m_state = AssetState::Loaded;
}

/*
 */
void
ScriptAsset::setSource(String source)
{
  m_source = std::move(source);
}

/*
 */
String
ScriptAsset::getChunkName() const
{
  String chunkName("@");
  chunkName += getFilePath().toString();
  return chunkName;
}

/*
 */
bool
ScriptAsset::serialize(SPtr<DataStream> stream)
{
  if (!stream) {
    return false;
  }
  // Kept in the text too, so the lines Lua reports match the file.
  UUID storedUUID = UUID::null();
  if (!readUUIDLine(m_source, storedUUID) || storedUUID != m_metadata.uuid) {
    m_source = withUUIDLine(m_source, m_metadata.uuid);
  }
  return stream->write(m_source.data(), m_source.size()) == m_source.size();
}

/*
 */
bool
ScriptAsset::deserialize(SPtr<DataStream> stream)
{
  if (!stream) {
    CH_LOG_ERROR(ScriptAssetLog, "Script {0} could not be read", m_metadata.name);
    return false;
  }
  // Read as text, so a file saved as UTF-16 or with a byte order mark still compiles.
  m_source = stream->getAsString();
  return true;
}

/*
 */
void
ScriptAsset::clearAssetData()
{
  m_source = String();
}

/*
 */
bool
ScriptAsset::readUUIDLine(StringView text, UUID& outUUID)
{
  if (!text.starts_with(UUID_LINE_PREFIX)) {
    return false;
  }
  StringView rest = text.substr(UUID_LINE_PREFIX.size());
  const SIZE_T lineEnd = rest.find_first_of("\r\n");
  if (lineEnd != StringView::npos) {
    rest = rest.substr(0, lineEnd);
  }
  return UUID::fromString(StringUtils::trimView(rest), outUUID);
}

/*
 */
String
ScriptAsset::withUUIDLine(StringView text, const UUID& uuid)
{
  // The file keeps the line endings it already uses.
  const StringView newLine = text.find("\r\n") != StringView::npos ? "\r\n" : "\n";
  UUID oldUUID = UUID::null();
  if (readUUIDLine(text, oldUUID)) {
    const SIZE_T lineEnd = text.find('\n');
    text = lineEnd == StringView::npos ? StringView() : text.substr(lineEnd + 1);
  }

  String result(UUID_LINE_PREFIX);
  result += uuid.toString();
  result += newLine;
  result += text;
  return result;
}

} // namespace chEngineSDK
