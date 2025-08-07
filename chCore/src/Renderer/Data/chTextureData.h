/************************************************************************/
/**
 * @file chTextureData.h
 * @author AccelMR
 * @date 2025/07/21
 * @brief Texture-based render data types
 */
/************************************************************************/
#pragma once


#include "chRenderDataBase.h"
#include "chTypeTraits.h"

namespace chEngineSDK {
class ITextureView;

// Declare type traits for all texture data types
struct AlbedoData;
DECLARE_TYPE_TRAITS(AlbedoData)
struct NormalData;
DECLARE_TYPE_TRAITS(NormalData)
struct DepthData;
DECLARE_TYPE_TRAITS(DepthData)
struct MetallicData;
DECLARE_TYPE_TRAITS(MetallicData)
struct RoughnessData;
DECLARE_TYPE_TRAITS(RoughnessData)
struct MotionVectorData;
DECLARE_TYPE_TRAITS(MotionVectorData)
struct EmissiveData;
DECLARE_TYPE_TRAITS(EmissiveData)

/**
 * @brief Base class for texture-based render data
 */
struct CH_CORE_EXPORT TextureData : public RenderStageData {
  SPtr<ITextureView> texture;

  bool isValid() const override {
    return texture != nullptr;
  }
};

/**
 * @brief Albedo color data from G-Buffer
 */
struct CH_CORE_EXPORT AlbedoData : public TextureData {
  UUID getTypeId() const override {
    return TypeTraits<AlbedoData>::getTypeId();
  }
};

/**
 * @brief World space normal data from G-Buffer
 */
struct CH_CORE_EXPORT NormalData : public TextureData {
  UUID getTypeId() const override {
    return TypeTraits<NormalData>::getTypeId();
  }
};

/**
 * @brief Depth buffer data
 */
struct CH_CORE_EXPORT DepthData : public TextureData {
  UUID getTypeId() const override {
    return TypeTraits<DepthData>::getTypeId();
  }
};


/**
 * @brief Metallic value from G-Buffer
 */
struct CH_CORE_EXPORT MetallicData : public TextureData {
  UUID getTypeId() const override {
    return TypeTraits<MetallicData>::getTypeId();
  }
};

/**
 * @brief Roughness value from G-Buffer
 */
struct CH_CORE_EXPORT RoughnessData : public TextureData {
  UUID getTypeId() const override {
    return TypeTraits<RoughnessData>::getTypeId();
  }
};
/**
 * @brief Motion vector data for temporal effects
 */
struct CH_CORE_EXPORT MotionVectorData : public TextureData {
  UUID getTypeId() const override {
    return TypeTraits<MotionVectorData>::getTypeId();
  }
};

/*
*/
struct CH_CORE_EXPORT EmissiveData : public TextureData {
  UUID getTypeId() const override {
    return TypeTraits<EmissiveData>::getTypeId();
  }
};
} // namespace chEngineSDK
