/************************************************************************/
/**
 * @file chSceneAsset.h
 * @author AccelMR
 * @date 2025/07/20
 * @brief
 *  A Scene saved as an asset, with references to what its objects use.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"
#include "chTypeTraits.h"
#include "chVector3.h"

namespace chEngineSDK {

/**
 * Exists to save a Scene and open it again. The file holds the GameObject hierarchy with its
 * components; the models and materials they use are its references, so they are loaded
 * before the scene. Loading makes a new Scene and unloading drops it.
 */
class CH_CORE_EXPORT SceneAsset : public IAsset
{
 public:
  SceneAsset() = delete;
  SceneAsset(const AssetMetadata& metadata) : IAsset(metadata) {}
  ~SceneAsset() override = default;

  /**
   * Null while the asset is not loaded.
   */
  NODISCARD FORCEINLINE const SPtr<Scene>&
  getScene() const noexcept { return m_scene; }

  /**
   * The scene saved from now on, such as a new scene saved for the first time.
   */
  void
  setScene(const SPtr<Scene>& scene);

#if USING(CH_EDITOR)
  /**
   * Where the editor camera was when the scene was saved, kept in its editor data. False
   * when it was never saved with one.
   */
  NODISCARD bool
  getEditorView(Vector3& outPosition, Vector3& outLookAt) const;

  void
  setEditorView(const Vector3& position, const Vector3& lookAt);
#endif // USING(CH_EDITOR)

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

#if USING(CH_EDITOR)
  void
  writeEditorValues(ConfigFile& file) const override;

  void
  readEditorValues(const ConfigFile& file) override;
#endif // USING(CH_EDITOR)

 private:
  SPtr<Scene> m_scene;
#if USING(CH_EDITOR)
  Vector3 m_editorViewPosition = Vector3::ZERO;
  Vector3 m_editorViewLookAt = Vector3::ZERO;
  bool m_bHasEditorView = false;
#endif // USING(CH_EDITOR)
};
DECLARE_ASSET_TYPE(SceneAsset);

} // namespace chEngineSDK
