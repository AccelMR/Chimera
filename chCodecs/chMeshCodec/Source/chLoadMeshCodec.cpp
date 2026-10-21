/************************************************************************/
/**
 * @file chLoadCodec.h
 * @author AccelMR
 * @date 2025/07/19
 * @brief
 * Chimera load codec header file.
 */
/************************************************************************/

#include "chPlatformDefines.h"
#include "chLogger.h"
#include "chAssetCodecManager.h"
#include "chMeshCodec.h"

using namespace chEngineSDK;

CH_LOG_DECLARE_STATIC(MeshCodecLog, All);

CH_EXTERN CH_PLUGIN_EXPORT void
loadPlugin() {
  CH_LOG_DEBUG(MeshCodecLog, "Loading Codec Plugin...");
  // Initialize the AssetCodecManager
  if (!AssetCodecManager::isStarted()){
    CH_LOG_ERROR(MeshCodecLog, "AssetCodecManager is not started. Cannot load codecs.");
    return;
  }
  AssetCodecManager::instance().registerCodec<chEngineSDK::MeshCodec>();
  CH_LOG_DEBUG(MeshCodecLog, "Mesh Codec Plugin loaded successfully.");
}
