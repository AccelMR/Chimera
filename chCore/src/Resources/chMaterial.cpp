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
#include "chTextureAsset.h"

namespace chEngineSDK {

/*
 */
void
Material::setBaseColorFactor(const LinearColor& color)
{
  m_baseColorFactor = color;
}

/*
 */
void
Material::setBaseColorTexture(const SPtr<TextureAsset>& texture)
{
  setBaseColorTexture(texture ? texture->getUUID() : UUID::null(), texture);
}

/*
 */
void
Material::setBaseColorTexture(const UUID& textureId, const SPtr<TextureAsset>& texture)
{
  m_baseColorTextureId = textureId;
  m_baseColorTexture = texture;
  m_baseColorGpuTexture = texture ? texture->getTexture().get() : nullptr;
}

/*
 */
void
Material::collectReferences(Vector<UUID>& outReferences) const
{
  if (!m_baseColorTextureId.isNull()) {
    outReferences.push_back(m_baseColorTextureId);
  }
}

/*
 */
void
Material::serialize(DataStream& stream) const
{
  const uint32 version = SERIALIZATION_VERSION;
  const float baseColor[4] = {
      m_baseColorFactor.r, m_baseColorFactor.g, m_baseColorFactor.b, m_baseColorFactor.a};
  stream.write(&version, sizeof(version));
  stream.write(baseColor, sizeof(baseColor));
  stream.write(&m_baseColorTextureId, sizeof(m_baseColorTextureId));
}

/*
 */
bool
Material::deserialize(DataStream& stream)
{
  uint32 version = 0;
  float baseColor[4];
  UUID baseColorTextureId;
  if (stream.read(&version, sizeof(version)) != sizeof(version) ||
      version != SERIALIZATION_VERSION ||
      stream.read(baseColor, sizeof(baseColor)) != sizeof(baseColor) ||
      stream.read(&baseColorTextureId, sizeof(baseColorTextureId)) !=
          sizeof(baseColorTextureId)) {
    return false;
  }

  m_baseColorFactor = LinearColor(baseColor[0], baseColor[1], baseColor[2], baseColor[3]);
  setBaseColorTexture(baseColorTextureId, nullptr);
  return true;
}

} // namespace chEngineSDK
