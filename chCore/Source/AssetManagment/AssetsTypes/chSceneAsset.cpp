/************************************************************************/
/**
 * @file chSceneAsset.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  A Scene saved as an asset, with references to what its objects use.
 */
/************************************************************************/
#include "chSceneAsset.h"

#include "chAlgorithm.h"
#include "chAssetFile.h"
#include "chAssetManager.h"
#include "chFileStream.h"
#include "chGameObjectSerializer.h"
#include "chLogger.h"
#include "chScene.h"

#if USING(CH_EDITOR)
#include <charconv>

#include "chConfigFile.h"
#include "chStringUtils.h"
#endif // USING(CH_EDITOR)

namespace chEngineSDK {

namespace {
// Above any real map; only a broken file asks for more.
constexpr uint32 kMaxSubLevels = 4096;

struct SubLevelFileData
{
  UUID asset;
  uint32 loadAtStart;
};
static_assert(sizeof(SubLevelFileData) == 20, "SubLevelFileData must have no padding");
} // namespace

#if USING(CH_EDITOR)
namespace {
constexpr StringView kEditorViewSection = "EditorView";

/*
 */
String
vectorText(const Vector3& vector)
{
  return StringUtils::format("{0} {1} {2}", vector.x, vector.y, vector.z);
}

/*
 * Three numbers separated by spaces; false, with outVector untouched, for anything else.
 */
bool
readVector(const ConfigFile& file, StringView key, Vector3& outVector)
{
  const String* text = file.getValue(kEditorViewSection, key);
  if (!text) {
    return false;
  }

  float values[3];
  const ANSICHAR* current = text->data();
  const ANSICHAR* const end = current + text->size();
  for (float& value : values) {
    while (current < end && *current == ' ') {
      ++current;
    }
    const std::from_chars_result result = std::from_chars(current, end, value);
    if (result.ec != std::errc()) {
      return false;
    }
    current = result.ptr;
  }
  outVector = Vector3(values[0], values[1], values[2]);
  return true;
}
} // namespace
#endif // USING(CH_EDITOR)

/*
 */
void
SceneAsset::setScene(const SPtr<Scene>& scene)
{
  m_scene = scene;
}

/*
 */
bool
SceneAsset::serialize(SPtr<DataStream> stream)
{
  if (!stream || !m_scene) {
    return false;
  }
  const uint32 version = SERIALIZATION_VERSION;
  stream->write(&version, sizeof(version));
  GameObjectSerializer::write(*stream, m_scene->getRootGameObjects());

  const Vector<SceneSubLevel>& subLevels = m_scene->getSubLevels();
  const uint32 subLevelCount = static_cast<uint32>(subLevels.size());
  stream->write(&subLevelCount, sizeof(subLevelCount));
  for (const SceneSubLevel& subLevel : subLevels) {
    const SubLevelFileData data{.asset = subLevel.asset,
                                .loadAtStart = subLevel.bLoadAtStart ? 1u : 0u};
    stream->write(&data, sizeof(data));
  }
  return true;
}

/*
 */
bool
SceneAsset::deserialize(SPtr<DataStream> stream)
{
  uint32 version = 0;
  if (!stream || stream->read(&version, sizeof(version)) != sizeof(version) ||
      version == 0 || version > SERIALIZATION_VERSION) {
    CH_LOG_ERROR(AssetSystem, "Scene {0} has a version this engine cannot read", getName());
    return false;
  }

  // IAsset::load already loaded the references, so the components find their assets.
  SPtr<Scene> scene = chMakeShared<Scene>(getName(), getUUID());
  if (!GameObjectSerializer::read(*stream, *scene, nullptr)) {
    CH_LOG_ERROR(AssetSystem, "Failed to read the objects of scene {0}", getName());
    return false;
  }

  uint32 subLevelCount = 0;
  if (!AssetFile::readValue(*stream, subLevelCount) || subLevelCount > kMaxSubLevels ||
      subLevelCount * sizeof(SubLevelFileData) > AssetFile::getBytesLeft(*stream)) {
    CH_LOG_ERROR(AssetSystem, "Scene {0} has a broken list of sublevels", getName());
    return false;
  }
  Vector<SceneSubLevel> subLevels(subLevelCount);
  for (SceneSubLevel& subLevel : subLevels) {
    SubLevelFileData data;
    if (!AssetFile::readValue(*stream, data)) {
      return false;
    }
    subLevel = {.asset = data.asset, .bLoadAtStart = data.loadAtStart != 0};
  }
  scene->setSubLevels(subLevels);
  scene->setMapLevel(getName(), getUUID());
  m_scene = std::move(scene);
  return true;
}

/*
 */
void
SceneAsset::clearAssetData()
{
  m_scene.reset();
}

/*
 */
void
SceneAsset::collectReferences(Vector<UUID>& outReferences) const
{
  if (m_scene) {
    GameObjectSerializer::collectReferences(m_scene->getRootGameObjects(), outReferences);
    // Named only: scenes are not loaded with their referrers, so this only packages them.
    for (const SceneSubLevel& subLevel : m_scene->getSubLevels()) {
      if (!subLevel.asset.isNull() && !Algorithm::contains(outReferences, subLevel.asset)) {
        outReferences.push_back(subLevel.asset);
      }
    }
  }
}

#if USING(CH_EDITOR)
/*
 */
bool
SceneAsset::getEditorView(Vector3& outPosition, Vector3& outLookAt) const
{
  if (!m_bHasEditorView) {
    return false;
  }
  outPosition = m_editorViewPosition;
  outLookAt = m_editorViewLookAt;
  return true;
}

/*
 */
void
SceneAsset::setEditorView(const Vector3& position, const Vector3& lookAt)
{
  m_editorViewPosition = position;
  m_editorViewLookAt = lookAt;
  m_bHasEditorView = true;
}

/*
 */
void
SceneAsset::writeEditorValues(ConfigFile& file) const
{
  if (m_bHasEditorView) {
    file.setValue(kEditorViewSection, "Position", vectorText(m_editorViewPosition));
    file.setValue(kEditorViewSection, "LookAt", vectorText(m_editorViewLookAt));
  }
}

/*
 */
void
SceneAsset::readEditorValues(const ConfigFile& file)
{
  Vector3 position;
  Vector3 lookAt;
  if (readVector(file, "Position", position) && readVector(file, "LookAt", lookAt)) {
    setEditorView(position, lookAt);
  }
}
#endif // USING(CH_EDITOR)

} // namespace chEngineSDK
