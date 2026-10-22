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
 * .lua that any editor opens. The UUID is a comment on the first line of the file
 * ("-- chimera-uuid: ..."), which Lua ignores; saving writes it, and the editor adds one to
 * a script that has none.
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

  /**
   * Reads the UUID from the first line of text; false when that line is not a UUID line.
   */
  NODISCARD static bool
  readUUIDLine(StringView text, UUID& outUUID);

  /**
   * text with its first line set to the UUID line of uuid: an old UUID line is replaced,
   * otherwise one is added before the first line.
   */
  NODISCARD static String
  withUUIDLine(StringView text, const UUID& uuid);

  static constexpr StringView EXTENSION = ".lua";
  static constexpr StringView UUID_LINE_PREFIX = "-- chimera-uuid: ";

 protected:
  bool
  serialize(SPtr<DataStream> stream) override;

  bool
  deserialize(SPtr<DataStream> stream) override;

  void
  clearAssetData() override;

 private:
  String m_source;
};
DECLARE_ASSET_TYPE(ScriptAsset);

} // namespace chEngineSDK
