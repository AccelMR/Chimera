/************************************************************************/
/**
 * @file chGraphicsModule.cpp
 * @author AccelMR
 * @date 2022/09/24
 *   
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chGraphicsModule.h"

#include "chGPUCommandBuffer.h"

namespace chEngineSDK{

/*
*/
void
GraphicsModule::initialize(const SPtr<Screen>& screen) {
  _internalInit(screen);
}

/*
*/
void
GraphicsModule::OnResize(uint32 width, uint32 height) {
  _internalOnResize(width, height);
}

/*
 *TODO: add command pool?
*/
SPtr<GPUCommandBuffer>
GraphicsModule::beginCommandRecording(COMMAND_BUFFER_TYPES commandBufferType, 
                                 const SPtr<GPUPipelineState>& pipelineState) {
  SPtr<GPUCommandBuffer> cmdBffr = _internalCreateGPUCommandBuffer();
  cmdBffr->_init(commandBufferType, pipelineState);
  return cmdBffr;
}

/*
*/
SPtr<GPUPipelineState>
GraphicsModule::createPipelineState(const PipelineStateDesc& pipelineStateDesc) {
  return _internalcreatePipelineState(pipelineStateDesc);
}

/*
*/
void
GraphicsModule::executeCommandBuffers(const Vector<SPtr<GPUCommandBuffer>>& commandBuffers) {
  _internalExecuteCommandBuffers(commandBuffers);
}

/*
*/
void
GraphicsModule::present(int32 syncInterval, int32 flags) {
  _internalPresent(syncInterval, flags);
}

/*
*/
SPtr<SwapChain>
GraphicsModule::getSwapChain() {
  return _internalGetSwapChain();
}

/*
*/
void
GraphicsModule::resetSwapChainAllocator() {
  _internalResetSwapChainAllocator();
}

/*
*/
SPtr<Fence>
GraphicsModule::createFence() {
  return _internalCreateFence();
}

/*
*/
void
GraphicsModule::syncGPU(const WPtr<Fence> fence, uint64 value) {
  _internalSyncGPU(fence, value);
}
} // namespace chEngineSDK


