/************************************************************************/
/**
 * @file chMaterialManager.cpp
 * @author AccelMR
 * @date 2025/08/02
 * @brief
 */
/************************************************************************/

#include "chMaterialManager.h"

#include "chIGraphicsAPI.h"
#include "chLogger.h"
#include "chIMaterial.h"
#include "chUUID.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(MaterialManagerLog, All);

namespace MaterialManagerConstants {

String
getStageName(ShaderStage stage) {
  switch (stage) {
  case ShaderStage::Vertex:
    return "Vertex";
  case ShaderStage::Fragment:
    return "Fragment";
  case ShaderStage::Compute:
    return "Compute";
  case ShaderStage::Geometry:
    return "Geometry";
  case ShaderStage::TessControl:
    return "TessControl";
  case ShaderStage::TessEvaluation:
    return "TessEvaluation";
  default:
    return "Unknown";
  }
}
} // namespace MaterialManagerConstants

/*
 */
SPtr<IMaterial>
MaterialManager::createCustomMaterial(const String&){
 return nullptr;
}

} // namespace chEngineSDK
