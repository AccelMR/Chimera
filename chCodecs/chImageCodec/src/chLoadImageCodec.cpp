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
#include "chImageCodec.h"

using namespace chEngineSDK;

CH_LOG_DECLARE_STATIC(ImageCodecLog, All);

CH_EXTERN CH_PLUGIN_EXPORT void
loadPlugin() {
  CH_LOG_DEBUG(ImageCodecLog, "Loading Codec Plugin...");
  // Initialize the AssetCodecManager
  if (!AssetCodecManager::isStarted()){
    CH_LOG_ERROR(ImageCodecLog, "AssetCodecManager is not started. Cannot load codecs.");
    return;
  }
  AssetCodecManager::instance().registerCodec<chEngineSDK::ImageCodec>();
  CH_LOG_DEBUG(ImageCodecLog, "Mesh Codec Plugin loaded successfully.");
}
