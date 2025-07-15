/************************************************************************/
/**
 * @file chModelAsset.cpp
 * @author AccelMR
 * @date 2025/07/14
 * @brief
 *  This file contains the ModelAsset class for managing model assets in the engine.
 */
/************************************************************************/
#include "chModelAsset.h"

#include "chLogger.h"

#include "chModelSerializationHeaders.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(ModelAssetLog, All);

/*
*/
bool
ModelAsset::serialize(SPtr<DataStream> stream) {
  if (!stream) {
    CH_LOG(ModelAssetLog, Error, "Data stream is null for ModelAsset serialization");
    return false;
  }

  // Serialize the model data here
  // For example, you might serialize the meshes, nodes, etc.
  // This is a placeholder implementation
  // stream->write(m_model->getRootNodes().size());
  // for (const auto& node : m_model->getRootNodes()) {
  //   stream->write(node->getName());
  //   // Serialize other properties of the node as needed
  // }

  CH_LOG(ModelAssetLog, Debug, "ModelAsset {0} serialized successfully", m_metadata.name);
  return true;
}

} // namespace chEngineSDK
