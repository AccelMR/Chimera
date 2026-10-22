/************************************************************************/
/**
 * @file chScriptAsset.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  A Lua script of the project.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"
#include "chTypeTraits.h"

namespace chEngineSDK {

/**
 * Exists so a script is an asset that scenes reference by UUID while its file stays a plain
 * .lua that any editor opens. The UUID lives in the .lua.meta file next to it; the editor
 * writes one for a script that has none.
 */
class CH_CORE_EXPORT ScriptAsset : public IAsset
{
 public:
  ScriptAsset() = delete;
  ScriptAsset(const AssetMetadata& metadata) : IAsset(metadata) {}
  /**
   * A script made from text, already loaded.
   */
  ScriptAsset(const AssetMetadata& metadata, String source);
  ~ScriptAsset() = default;

  /**
   * The text of the script; empty while it is not loaded.
   */
  NODISCARD FORCEINLINE const String&
  getSource() const noexcept { return m_source; }

  void
  setSource(String source);

  /**
   * "@<virtual path>", the name Lua shows in errors and tracebacks.
   */
  NODISCARD String
  getChunkName() const;

  NODISCARD StringView
  getFileExtension() const override { return EXTENSION; }

  NODISCARD bool
  isPlainFile() const override { return true; }

  static constexpr StringView EXTENSION = ".lua";
  // Where the .meta file keeps the UUID.
  static constexpr StringView META_SECTION = "Asset";
  static constexpr StringView META_UUID_KEY = "UUID";

 protected:
  bool
  serialize(SPtr<DataStream> stream) override;

  bool
  deserialize(SPtr<DataStream> stream) override;

  void
  clearAssetData() override;

#if USING(CH_EDITOR)
  void
  writeEditorValues(ConfigFile& file) const override;
#endif // USING(CH_EDITOR)

 private:
  String m_source;
};
DECLARE_ASSET_TYPE(ScriptAsset);

} // namespace chEngineSDK
