/************************************************************************/
/**
 * @file chPrefabAsset.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 *  A GameObject hierarchy saved as an asset, to be placed many times.
 */
/************************************************************************/
#include "chPrefabAsset.h"

#include <cstring>

#include "chAssetManager.h"
#include "chFileStream.h"
#include "chGameObject.h"
#include "chGameObjectSerializer.h"
#include "chLogger.h"

namespace chEngineSDK {
namespace {
// Above any real prefab; only a broken file asks for more.
constexpr uint32 kMaxContentSize = 1u << 30;
// Prefabs inside prefabs deeper than this are taken for a loop.
constexpr uint32 kMaxNesting = 256;

/*
 */
bool
dependsOnRecursive(const Vector<UUID>& references, const UUID& prefab, uint32 depth)
{
  if (depth > kMaxNesting) {
    return true;
  }
  for (const UUID& reference : references) {
    if (reference == prefab) {
      return true;
    }
    if (!AssetManager::isStarted()) {
      continue;
    }
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(reference);
    const SPtr<PrefabAsset> nested = asset ? asset->as<PrefabAsset>() : nullptr;
    if (nested && dependsOnRecursive(nested->getReferencedAssets(), prefab, depth + 1)) {
      return true;
    }
  }
  return false;
}
} // namespace

/*
 */
bool
PrefabAsset::setContent(const GameObject& root)
{
  // An instance in the content, the root included, is one of its references.
  Vector<UUID> references;
  GameObjectSerializer::collectReferences(root, references);
  if (dependsOnRecursive(references, getUUID(), 0)) {
    CH_LOG_ERROR(AssetSystem, "Prefab {0} cannot hold an instance of itself.", getName());
    return false;
  }

  MemoryDataStream stream;
  GameObjectSerializer::write(stream, root);
  m_content.resize(stream.size());
  if (!m_content.empty()) {
    std::memcpy(m_content.data(), stream.getStartPtr(), m_content.size());
  }
  m_contentReferences = std::move(references);
  return true;
}

/*
 */
bool
PrefabAsset::dependsOn(const UUID& prefab) const
{
  return getUUID() == prefab || dependsOnRecursive(m_contentReferences, prefab, 0);
}

/*
 */
bool
PrefabAsset::serialize(SPtr<DataStream> stream)
{
  if (!stream) {
    return false;
  }
  const uint32 version = SERIALIZATION_VERSION;
  const uint32 size = static_cast<uint32>(m_content.size());
  stream->write(&version, sizeof(version));
  stream->write(&size, sizeof(size));
  stream->write(m_content.data(), m_content.size());
  return true;
}

/*
 */
bool
PrefabAsset::deserialize(SPtr<DataStream> stream)
{
  uint32 version = 0;
  uint32 size = 0;
  if (!stream || stream->read(&version, sizeof(version)) != sizeof(version) ||
      version == 0 || version > SERIALIZATION_VERSION ||
      stream->read(&size, sizeof(size)) != sizeof(size) || size > kMaxContentSize) {
    CH_LOG_ERROR(AssetSystem, "Prefab {0} has data this engine cannot read", getName());
    return false;
  }
  m_content.resize(size);
  if (stream->read(m_content.data(), size) != size) {
    CH_LOG_ERROR(AssetSystem, "Prefab {0} is cut short", getName());
    m_content.clear();
    return false;
  }
  // IAsset::load read them from the start of the file.
  m_contentReferences = m_referencedAssets;
  return true;
}

/*
 */
void
PrefabAsset::clearAssetData()
{
  m_content.clear();
  m_contentReferences.clear();
}

/*
 */
void
PrefabAsset::collectReferences(Vector<UUID>& outReferences) const
{
  outReferences.insert(outReferences.end(), m_contentReferences.begin(),
                       m_contentReferences.end());
}

} // namespace chEngineSDK
