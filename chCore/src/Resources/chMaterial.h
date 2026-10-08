/************************************************************************/
/**
 * @file chMaterial.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  How a surface looks: colors and textures the renderer reads per mesh.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chLinearColor.h"
#include "chUUID.h"

namespace chEngineSDK {
class TextureAsset;

/**
 * Exists to keep how a surface looks apart from its shape, so many meshes can share it and
 * the renderer reads one place per draw. It keeps the UUID of each texture even when that
 * asset is missing, so saving it again does not lose the reference.
 */
class CH_CORE_EXPORT Material
{
 public:
  void
  setBaseColorFactor(const LinearColor& color);

  NODISCARD FORCEINLINE const LinearColor&
  getBaseColorFactor() const noexcept { return m_baseColorFactor; }

  /**
   * Null clears it.
   */
  void
  setBaseColorTexture(const SPtr<TextureAsset>& texture);

  /**
   * Used while loading: keeps the id even when the asset is missing (null).
   */
  void
  setBaseColorTexture(const UUID& textureId, const SPtr<TextureAsset>& texture);

  NODISCARD FORCEINLINE const UUID&
  getBaseColorTextureId() const noexcept { return m_baseColorTextureId; }

  NODISCARD FORCEINLINE const SPtr<TextureAsset>&
  getBaseColorTexture() const noexcept { return m_baseColorTexture; }

  /**
   * Null when there is no texture or it has no GPU copy; the renderer then uses its default.
   */
  NODISCARD FORCEINLINE const ITexture*
  getBaseColorGpuTexture() const noexcept { return m_baseColorGpuTexture; }

  void
  collectReferences(Vector<UUID>& outReferences) const;

  void
  serialize(DataStream& stream) const;

  /**
   * Reads the values and texture ids; the textures stay null until set by the caller, which
   * knows where to find the assets.
   */
  NODISCARD bool
  deserialize(DataStream& stream);

  static constexpr uint32 SERIALIZATION_VERSION = 1;

 private:
  LinearColor m_baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  UUID m_baseColorTextureId = UUID::null();
  SPtr<TextureAsset> m_baseColorTexture;
  // Taken from the texture asset when it is set, so a draw does not go through the asset.
  const ITexture* m_baseColorGpuTexture = nullptr;
};

} // namespace chEngineSDK
