/************************************************************************/
/**
 * @file chMaterial.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  How a surface looks: colors and textures the renderer reads per mesh.
 */
/************************************************************************/
#include "chMaterial.h"

#include "chFileStream.h"
#include "chIBuffer.h"
#include "chIGraphicsAPI.h"
#include "chITexture.h"
#include "chTextureAsset.h"

namespace chEngineSDK {

namespace {
// Must match MaterialData in chMaterial.hlsli.
struct MaterialConstants
{
  float baseColorFactor[4];
  float emissiveFactor[3];
  float metallicFactor;
  float roughnessFactor;
  float normalScale;
  float occlusionStrength;
  uint32 baseColorTexture;
  uint32 metallicRoughnessTexture;
  uint32 normalTexture;
  uint32 emissiveTexture;
  uint32 occlusionTexture;
};
static_assert(sizeof(MaterialConstants) == 64);

// Must match kNoTexture in chMaterial.hlsli: shaders skip the sample and use the factor.
constexpr uint32 kNoTexture = 0xFFFFFFFF;

constexpr uint32 kTextureCount = static_cast<uint32>(MaterialTexture::COUNT);

// What is written for each value after the version.
struct MaterialFileData
{
  float baseColorFactor[4];
  float emissiveFactor[3];
  float metallicFactor;
  float roughnessFactor;
  float normalScale;
  float occlusionStrength;
  UUID textures[kTextureCount];
};
static_assert(sizeof(MaterialFileData) == 44 + sizeof(UUID) * kTextureCount,
              "MaterialFileData must have no padding");
} // namespace

/*
 */
void
Material::setBaseColorFactor(const LinearColor& color)
{
  m_baseColorFactor = color;
  m_bConstantsDirty = true;
}

/*
 */
void
Material::setMetallicFactor(float metallic)
{
  m_metallicFactor = metallic;
  m_bConstantsDirty = true;
}

/*
 */
void
Material::setRoughnessFactor(float roughness)
{
  m_roughnessFactor = roughness;
  m_bConstantsDirty = true;
}

/*
 */
void
Material::setNormalScale(float scale)
{
  m_normalScale = scale;
  m_bConstantsDirty = true;
}

/*
 */
void
Material::setOcclusionStrength(float strength)
{
  m_occlusionStrength = strength;
  m_bConstantsDirty = true;
}

/*
 */
void
Material::setEmissiveFactor(const LinearColor& color)
{
  m_emissiveFactor = color;
  m_bConstantsDirty = true;
}

/*
 */
void
Material::setTexture(MaterialTexture slot, const SPtr<TextureAsset>& texture)
{
  setTexture(slot, texture ? texture->getUUID() : UUID::null(), texture);
}

/*
 */
void
Material::setTexture(MaterialTexture slot,
                     const UUID& textureId,
                     const SPtr<TextureAsset>& texture)
{
  CH_ASSERT(slot < MaterialTexture::COUNT);
  TextureSlot& textureSlot = m_textures[static_cast<uint32>(slot)];
  textureSlot.id = textureId;
  textureSlot.asset = texture;
  textureSlot.gpuTexture = texture ? texture->getTexture().get() : nullptr;
  m_bConstantsDirty = true;
}

/*
 */
const IBuffer&
Material::getConstantBuffer() const
{
  if (!m_constantBuffer) {
    m_constantBuffer =
        IGraphicsAPI::instance().createBuffer({.size = sizeof(MaterialConstants),
                                               .usage = BufferUsage::UniformBuffer,
                                               .memoryUsage = MemoryUsage::GpuOnly});
    m_bConstantsDirty = true;
  }
  if (m_bConstantsDirty) {
    updateConstantBuffer();
  }
  return *m_constantBuffer;
}

/*
 */
void
Material::updateConstantBuffer() const
{
  uint32 textureIndexes[kTextureCount];
  for (uint32 i = 0; i < kTextureCount; ++i) {
    const ITexture* texture = m_textures[i].gpuTexture;
    textureIndexes[i] = texture ? texture->getBindlessIndex() : kNoTexture;
  }

  const MaterialConstants constants{
      .baseColorFactor = {m_baseColorFactor.r, m_baseColorFactor.g, m_baseColorFactor.b,
                          m_baseColorFactor.a},
      .emissiveFactor = {m_emissiveFactor.r, m_emissiveFactor.g, m_emissiveFactor.b},
      .metallicFactor = m_metallicFactor,
      .roughnessFactor = m_roughnessFactor,
      .normalScale = m_normalScale,
      .occlusionStrength = m_occlusionStrength,
      .baseColorTexture = textureIndexes[static_cast<uint32>(MaterialTexture::BaseColor)],
      .metallicRoughnessTexture =
          textureIndexes[static_cast<uint32>(MaterialTexture::MetallicRoughness)],
      .normalTexture = textureIndexes[static_cast<uint32>(MaterialTexture::Normal)],
      .emissiveTexture = textureIndexes[static_cast<uint32>(MaterialTexture::Emissive)],
      .occlusionTexture = textureIndexes[static_cast<uint32>(MaterialTexture::Occlusion)]};
  m_constantBuffer->update(&constants, sizeof(constants));
  m_bConstantsDirty = false;
}

/*
 */
void
Material::collectReferences(Vector<UUID>& outReferences) const
{
  for (const TextureSlot& textureSlot : m_textures) {
    if (!textureSlot.id.isNull()) {
      outReferences.push_back(textureSlot.id);
    }
  }
}

/*
 */
void
Material::serialize(DataStream& stream) const
{
  const uint32 version = SERIALIZATION_VERSION;
  MaterialFileData data{
      .baseColorFactor = {m_baseColorFactor.r, m_baseColorFactor.g, m_baseColorFactor.b,
                          m_baseColorFactor.a},
      .emissiveFactor = {m_emissiveFactor.r, m_emissiveFactor.g, m_emissiveFactor.b},
      .metallicFactor = m_metallicFactor,
      .roughnessFactor = m_roughnessFactor,
      .normalScale = m_normalScale,
      .occlusionStrength = m_occlusionStrength,
      .textures = {}};
  for (uint32 i = 0; i < kTextureCount; ++i) {
    data.textures[i] = m_textures[i].id;
  }
  stream.write(&version, sizeof(version));
  stream.write(&data, sizeof(data));
}

/*
 */
bool
Material::deserialize(DataStream& stream)
{
  uint32 version = 0;
  MaterialFileData data;
  if (stream.read(&version, sizeof(version)) != sizeof(version) ||
      version != SERIALIZATION_VERSION || stream.read(&data, sizeof(data)) != sizeof(data)) {
    return false;
  }

  const float* baseColor = data.baseColorFactor;
  const float* emissive = data.emissiveFactor;
  m_baseColorFactor = LinearColor(baseColor[0], baseColor[1], baseColor[2], baseColor[3]);
  m_emissiveFactor = LinearColor(emissive[0], emissive[1], emissive[2], 1.0f);
  m_metallicFactor = data.metallicFactor;
  m_roughnessFactor = data.roughnessFactor;
  m_normalScale = data.normalScale;
  m_occlusionStrength = data.occlusionStrength;
  for (uint32 i = 0; i < kTextureCount; ++i) {
    setTexture(static_cast<MaterialTexture>(i), data.textures[i], nullptr);
  }
  m_bConstantsDirty = true;
  return true;
}

} // namespace chEngineSDK
