#include "chPrerequisitesDX12.h"

#include "chDebug.h"
#include "chDX12GraphicsModule.h"
#include "chDX12GPUResourceModule.h"

using namespace chEngineSDK;

CH_EXTERN CH_PLUGIN_EXPORT void
loadPlugin(const SPtr<Screen> _screen) {
  GraphicsModule::startUp<GraphicsModuleDX12>(_screen);
  GPUResourceModule::startUp<DX12GPUResourceModule>();

}