/************************************************************************/
/**
 * @file chSceneData.h
 * @author AccelMR
 * @date 2025/07/21
 * @brief Scene object render data types (cameras, models, lights)
 */
/************************************************************************/
#pragma once

#include "chRenderDataBase.h"
#include "chCamera.h"
#include "chModel.h"
#include "chTypeTraits.h"

namespace chEngineSDK {

// Declare type traits
struct CameraData;
DECLARE_TYPE_TRAITS(CameraData)
struct ModelData;
DECLARE_TYPE_TRAITS(ModelData)

/**
 * @brief Camera data for rendering
 */
struct CH_CORE_EXPORT CameraData : public RenderStageData {
  SPtr<Camera> camera;

  UUID getTypeId() const override {
    return TypeTraits<CameraData>::getTypeId();
  }

  bool isValid() const override {
    return camera != nullptr;
  }
};

/**
 * @brief Model data for rendering
 */
struct CH_CORE_EXPORT ModelData : public RenderStageData {
  SPtr<Model> model;

  UUID getTypeId() const override {
    return TypeTraits<ModelData>::getTypeId();
  }

  bool isValid() const override {
    return model != nullptr;
  }
};

} // namespace chEngineSDK
