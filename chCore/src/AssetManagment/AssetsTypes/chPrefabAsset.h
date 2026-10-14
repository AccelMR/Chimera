/************************************************************************/
/**
 * @file chPrefabAsset.h
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 *  A GameObject hierarchy saved as an asset, to be placed many times.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"
#include "chTypeTraits.h"

namespace chEngineSDK {

/**
 * Exists to save a GameObject with its components and children once and place it in many
 * scenes. Each placed copy is an instance that keeps only a link to the prefab, so changing
 * the prefab changes every instance the next time it loads. The content is kept as the bytes
 * GameObjectSerializer writes, so an instance is made by reading them, with no copy code of
 * its own.
 */
class CH_CORE_EXPORT PrefabAsset : public IAsset
{
 public:
  PrefabAsset() = delete;
  PrefabAsset(const AssetMetadata& metadata) : IAsset(metadata) {}
  ~PrefabAsset() override = default;

  /**
   * Takes root and its children as the new content; save() writes it to the file. The root's
   * own transform is left out, because an instance stands where it is placed. False, with
   * the content unchanged, when root holds an instance of this prefab (directly or through
   * another prefab).
   */
  NODISCARD bool
  setContent(const GameObject& root);

  /**
   * Empty while the asset is not loaded or has no content.
   */
  NODISCARD FORCEINLINE Span<const uint8>
  getContent() const noexcept { return m_content; }

  /**
   * True when this prefab is prefab, or holds an instance of it at any depth.
   */
  NODISCARD bool
  dependsOn(const UUID& prefab) const;

  static constexpr uint32 SERIALIZATION_VERSION = 1;

 protected:
  bool
  serialize(SPtr<DataStream> stream) override;

  bool
  deserialize(SPtr<DataStream> stream) override;

  void
  clearAssetData() override;

  void
  collectReferences(Vector<UUID>& outReferences) const override;

 private:
  Vector<uint8> m_content;
  // What the content uses, kept so saving again does not need to read it.
  Vector<UUID> m_contentReferences;
};
DECLARE_ASSET_TYPE(PrefabAsset);

} // namespace chEngineSDK
