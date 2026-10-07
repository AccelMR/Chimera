/************************************************************************/
/**
 * @file chLoadGraphicsAPI.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Entry point of the Direct3D 12 plugin.
 */
/************************************************************************/
#include "chDX12API.h"

using namespace chEngineSDK;

CH_EXTERN CH_PLUGIN_EXPORT void
loadPlugin()
{
  CH_LOG_DEBUG(DX12, "Loading Direct3D 12 plugin...");
  IGraphicsAPI::startUp<DX12API>();
}

namespace chEngineSDK {
CH_LOG_DEFINE_CATEGORY_SHARED(DX12, All);
} // namespace chEngineSDK
