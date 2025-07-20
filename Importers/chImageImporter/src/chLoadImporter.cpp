/************************************************************************/
/**
 * @file chLoadImporter.h
 * @author AccelMR
 * @date 2025/07/19
 * @brief
 * Chimera load importer header file.
 */
/************************************************************************/

#include "chPlatformDefines.h"
#include "chLogger.h"
#include "chAssetImporterManager.h"
#include "chImageImporter.h"

using namespace chEngineSDK;

CH_LOG_DECLARE_STATIC(ImageImporterLog, All);

CH_EXTERN CH_PLUGIN_EXPORT void
loadPlugin() {
  CH_LOG_DEBUG(ImageImporterLog, "Loading Importer Plugin...");
  // Initialize the AssetImporterManager
  if (!AssetImporterManager::isStarted()){
    CH_LOG_ERROR(ImageImporterLog, "AssetImporterManager is not started. Cannot load importers.");
    return;
  }
  AssetImporterManager::instance().registerImporter<chEngineSDK::ImageImporter>();
  CH_LOG_DEBUG(ImageImporterLog, "Mesh Importer Plugin loaded successfully.");
}
