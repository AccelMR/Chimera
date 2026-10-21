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
 * Texture inputs of a metallic-roughness material. Their channels follow glTF:
 * MetallicRoughness keeps roughness in green and metallic in blue, Occlusion reads red, and
 * Normal is a tangent space map with green pointing up the image. Material files store one
 * texture per value in this order, so the values are fixed.
 */
enum class MaterialTexture : uint32
{
  BaseColor = 0,
  MetallicRoughness = 1,
  Normal = 2,
  Emissive = 3,
  Occlusion = 4,

  COUNT
};

/**
 * How the alpha of the base color is used. Stored in material files, so the values are
 * fixed.
 */
enum class MaterialAlphaMode : uint32
{
  // The alpha is ignored.
  Opaque = 0,
  // Pixels whose alpha is under the cutoff are not drawn (leaves, fences, chains).
  Mask = 1,

  COUNT
};

/**
 * Exists to keep how a surface looks apart from its shape, so many meshes can share it and
 * the renderer reads one place per draw. It keeps the UUID of each texture even when that
 * asset is missing, so saving it again does not lose the reference.
 */
class CH_CORE_EXPORT Material
{
 public:
  Material() = default;
  Material(const Material&) = delete;
  Material(Material&&) noexcept = default;
  ~Material() = default;

  Material&
  operator=(const Material&) = delete;

  Material&
  operator=(Material&&) noexcept = default;

  void
  setBaseColorFactor(const LinearColor& color);

  NODISCARD FORCEINLINE const LinearColor&
  getBaseColorFactor() const noexcept { return m_baseColorFactor; }

  void
  setMetallicFactor(float metallic);

  NODISCARD FORCEINLINE float
  getMetallicFactor() const noexcept { return m_metallicFactor; }

  void
  setRoughnessFactor(float roughness);

  NODISCARD FORCEINLINE float
  getRoughnessFactor() const noexcept { return m_roughnessFactor; }

  void
  setNormalScale(float scale);

  NODISCARD FORCEINLINE float
  getNormalScale() const noexcept { return m_normalScale; }

  void
  setOcclusionStrength(float strength);

  NODISCARD FORCEINLINE float
  getOcclusionStrength() const noexcept { return m_occlusionStrength; }

  /**
   * Only red, green and blue are used.
   */
  void
  setEmissiveFactor(const LinearColor& color);

  NODISCARD FORCEINLINE const LinearColor&
  getEmissiveFactor() const noexcept { return m_emissiveFactor; }

  void
  setAlphaMode(MaterialAlphaMode mode);

  NODISCARD FORCEINLINE MaterialAlphaMode
  getAlphaMode() const noexcept { return m_alphaMode; }

  /**
   * Only used by MaterialAlphaMode::Mask.
   */
  void
  setAlphaCutoff(float cutoff);

  NODISCARD FORCEINLINE float
  getAlphaCutoff() const noexcept { return m_alphaCutoff; }

  /**
   * Draws the back of every triangle too, lit with the normal turned around, for surfaces
   * that have no thickness (leaves, cloth).
   */
  void
  setTwoSided(bool bTwoSided);

  NODISCARD FORCEINLINE bool
  isTwoSided() const noexcept { return m_bTwoSided; }

  /**
   * Null clears it.
   */
  void
  setTexture(MaterialTexture slot, const SPtr<TextureAsset>& texture);

  /**
   * Used while loading: keeps the id even when the asset is missing (null).
   */
  void
  setTexture(MaterialTexture slot, const UUID& textureId, const SPtr<TextureAsset>& texture);

  NODISCARD FORCEINLINE const UUID&
  getTextureId(MaterialTexture slot) const noexcept
  {
    return m_textures[static_cast<uint32>(slot)].id;
  }

  NODISCARD FORCEINLINE const SPtr<TextureAsset>&
  getTexture(MaterialTexture slot) const noexcept
  {
    return m_textures[static_cast<uint32>(slot)].asset;
  }

  /**
   * Null when there is no texture or it has no GPU copy.
   */
  NODISCARD FORCEINLINE const ITexture*
  getGpuTexture(MaterialTexture slot) const noexcept
  {
    return m_textures[static_cast<uint32>(slot)].gpuTexture;
  }

  /**
   * GPU copy of the factors and texture indexes that shaders read by bindless index, made
   * on the first call and sent again after a change (the copy runs with the next frame
   * submit). Main thread only.
   */
  NODISCARD const IBuffer&
  getConstantBuffer() const;

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

  static constexpr uint32 SERIALIZATION_VERSION = 3;

 private:
  struct TextureSlot
  {
    UUID id = UUID::null();
    SPtr<TextureAsset> asset;
    // Taken from the texture asset when it is set, so a draw does not go through the asset.
    const ITexture* gpuTexture = nullptr;
  };

  void
  updateConstantBuffer() const;

  // Defaults of a plain dielectric, so a model without PBR data does not look like metal.
  LinearColor m_baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  LinearColor m_emissiveFactor{0.0f, 0.0f, 0.0f, 1.0f};
  float m_metallicFactor = 0.0f;
  float m_roughnessFactor = 0.5f;
  float m_normalScale = 1.0f;
  float m_occlusionStrength = 1.0f;
  // glTF's default cutoff.
  float m_alphaCutoff = 0.5f;
  MaterialAlphaMode m_alphaMode = MaterialAlphaMode::Opaque;
  bool m_bTwoSided = false;
  TextureSlot m_textures[static_cast<uint32>(MaterialTexture::COUNT)];

  mutable SPtr<IBuffer> m_constantBuffer;
  mutable bool m_bConstantsDirty = true;
};

} // namespace chEngineSDK
