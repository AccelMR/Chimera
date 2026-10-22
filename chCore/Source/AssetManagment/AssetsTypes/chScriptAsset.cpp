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

#if USING(CH_EDITOR)
#include "chConfigFile.h"
#endif // USING(CH_EDITOR)

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

#if USING(CH_EDITOR)
/*
 */
void
ScriptAsset::writeEditorValues(ConfigFile& file) const
{
  file.setValue(META_SECTION, META_UUID_KEY, m_metadata.uuid.toString());
}
#endif // USING(CH_EDITOR)

} // namespace chEngineSDK
