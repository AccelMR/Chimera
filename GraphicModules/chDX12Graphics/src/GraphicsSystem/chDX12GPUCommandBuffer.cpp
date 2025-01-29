/************************************************************************/
/**
 * @file chDX12GPUCommandBuffer.cpp
 * @author AccelMR
 * @date 2022/09/28
 *   GPU Command buffer. Used to send instructions to the GPU to be
 *        executed at the render pass.
 *
 * DirectX implementation specific.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12GPUCommandBuffer.h"

#include "chLinearColor.h"

#include "chDX12GraphicsModule.h"
#include "chDX12GPUPipelineState.h"

#include "chDX12Texture.h"
#include "chDX12SwapChain.h"
#include "chDX12GPUBuffer.h"
#include "chDX12VertexBuffer.h"
#include "chDX12IndexBuffer.h"

#include "chDX12Translator.h"
#include "chBox2D.h"

namespace chEngineSDK{
/*
*/
DX12GPUCommandBuffer::~DX12GPUCommandBuffer() {
  SafeRelease(m_commandList);
}

/*
*/
void
DX12GPUCommandBuffer::_init(COMMAND_BUFFER_TYPES commandBufferType, 
                            const SPtr<GPUPipelineState>& pipelineState) {
  rhiModule = static_cast<GraphicsModuleDX12*>(GraphicsModule::instancePtr());
  ID3D12Device* device = rhiModule->getDevice();
  auto pipeline = pipelineState ?
    std::reinterpret_pointer_cast<DX12GPUPipelineState>(pipelineState)->m_pipeline :
    nullptr;

  auto dxCommandBuffType = D3D12_COMMAND_LIST_TYPE_DIRECT;

  // Create the command list.
  ThrowIfFailed(device->CreateCommandList(0,
                                         dxCommandBuffType,
                                         rhiModule->m_frameAlloc[rhiModule->m_frameIndex],
                                         pipeline,
                                         IID_PPV_ARGS(&m_commandList)));
  ThrowIfFailed(m_commandList->Close());
}

/*
*/
void
DX12GPUCommandBuffer::_internalReset(const SPtr<GPUPipelineState>& pipelineState){
  auto dx12Pipeline = std::reinterpret_pointer_cast<DX12GPUPipelineState>(pipelineState);
  ThrowIfFailed(rhiModule->m_frameAlloc[rhiModule->m_frameIndex]->Reset());

  ThrowIfFailed(m_commandList->Reset(rhiModule->m_frameAlloc[rhiModule->m_frameIndex],
                                     dx12Pipeline ? dx12Pipeline->m_pipeline: nullptr));
}

/*
*/
void
DX12GPUCommandBuffer::_internalclose() {
  ThrowIfFailed(m_commandList->Close());
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetGPUBuffer(const SPtr<GPUBuffer>& buff, uint32 slot) {
  auto dx12Buff = std::reinterpret_pointer_cast<DX12GPUBuffer>(buff);
  m_commandList->SetGraphicsRootConstantBufferView(slot, dx12Buff->m_buffer->GetGPUVirtualAddress());
}

/*
*/Allerendys

void
DX12GPUCommandBuffer::_internalSetGPUBuffers(const Vector<SPtr<GPUBuffer>>& buff, uint32 slot) {
  throw std::logic_error("The method or operation is not implemented." );
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetRect(const Box2D& rect) {
  auto size = rect.getSize();
  CD3DX12_VIEWPORT vp(static_cast<float>(rect.minPoint.x),
                      static_cast<float>(rect.minPoint.y),
                      size.x, size.y);
  m_commandList->RSSetViewports(1, &vp);
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetScissorRect(const Box2D& rect) {
  auto size = rect.getSize();
  CD3DX12_RECT dx12Rect(static_cast<float>(rect.minPoint.x),
                        static_cast<float>(rect.minPoint.y),
                        size.x, size.y);
  //CD3DX12_RECT dx12Rect(0, 0, LONG_MAX, LONG_MAX);
  m_commandList->RSSetScissorRects(1, &dx12Rect);
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetRenderTarget(const SPtr<Texture>& rt) {
  auto dx12Text = std::reinterpret_pointer_cast<DX12Texture>(rt);
  m_commandList->OMSetRenderTargets(1, &dx12Text->m_rtv, FALSE, nullptr);
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetRenderTargets(const Vector<SPtr<Texture>> &rts ) {
  throw std::logic_error("The method or operation is not implemented." );
}

/*
*/
void
DX12GPUCommandBuffer::_internalClearRenderTarget(const SPtr<Texture>& rt, const LinearColor &color ) {
  auto dx12Text = std::reinterpret_pointer_cast<DX12Texture>(rt);
  m_commandList->ClearRenderTargetView(dx12Text->m_rtv, color.rgba, 0, nullptr);
}

/*
*/
void
DX12GPUCommandBuffer::_internalClearRenderTargets(const Vector<SPtr<Texture>> &rts, const LinearColor &color ) {
  throw std::logic_error("The method or operation is not implemented." );
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetTopology(chGPUDesc::PRIMITIVE_TOPOLOGY_TYPE topology) {
  m_commandList->IASetPrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(DX12Translator::get1(topology)));
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetVertexBuffer(uint32 startSlot, 
                                               uint32 numViews, 
                                               const SPtr<VertexBuffer>& vertexBuff) {
  auto dx12VB = std::reinterpret_pointer_cast<DX12VertexBuffer>(vertexBuff);
  m_commandList->IASetVertexBuffers(startSlot, numViews, &dx12VB->m_vertexBufferView);
}

/*
*/
void
DX12GPUCommandBuffer::_internalDrawInstanced(uint32 vertexCountPerInstance, 
                                             uint32 instanceCount,
                                             uint32 startVertexLocation, 
                                             uint32 startInstanceLocation) {
  m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, 
                               startVertexLocation, startInstanceLocation);
}

/*
*/
void
DX12GPUCommandBuffer::_internalResourceBarrier(const Vector<chGPUDesc::GPUBarrier>& barriers) {
  for (const auto& barrier: barriers) {
    auto dx12Text = std::reinterpret_pointer_cast<DX12Texture>(barrier.transition.resource);
    auto barResource = CD3DX12_RESOURCE_BARRIER::Transition(dx12Text->m_resource,
                                                            DX12Translator::get(barrier.transition.stateBefore),
                                                            DX12Translator::get(barrier.transition.stateAfter));
    m_commandList->ResourceBarrier(1, &barResource);
  }


}

/*
*/
void
DX12GPUCommandBuffer::_internalClearSwapChainTexture(const LinearColor& color) {
  uint32 m_rtvDescriptorSize =
  rhiModule->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  auto swapchain  = std::reinterpret_pointer_cast<DX12SwapChain>(rhiModule->getSwapChain());

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
  swapchain->m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), 
  rhiModule->m_frameIndex, 
  m_rtvDescriptorSize);

  m_commandList->ClearRenderTargetView(rtvHandle, color.rgba, 0, nullptr);
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetSwapChain() {
  auto swapchain = std::reinterpret_pointer_cast<DX12SwapChain>(rhiModule->getSwapChain());
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapchain->m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                                          rhiModule->m_frameIndex, 
                                          swapchain->m_rtvDescriptorSize);
  m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}

/*
*/
void
DX12GPUCommandBuffer::_internalResourceBarrierSwapChain(const chGPUDesc::GPUBarrier& barrier) {
  auto swapchain = std::reinterpret_pointer_cast<DX12SwapChain>(rhiModule->getSwapChain());
  auto transition = CD3DX12_RESOURCE_BARRIER::Transition(swapchain->m_renderTargets[rhiModule->m_frameIndex],
                                                 DX12Translator::get(barrier.transition.stateBefore),
                                                 DX12Translator::get(barrier.transition.stateAfter));
  m_commandList->ResourceBarrier(1, &transition);
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetPipeLineState(const SPtr<GPUPipelineState>& pipelineState) {
  auto dx12Pipeline = std::reinterpret_pointer_cast<DX12GPUPipelineState>(pipelineState);
  m_commandList->SetPipelineState(dx12Pipeline->m_pipeline);
  m_commandList->SetGraphicsRootSignature(dx12Pipeline->m_rs);
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetBindingBufferGroup(const chGPUDesc::BindingGroup& bindingGroup) {
  auto rs = DX12Translator::get(bindingGroup, rhiModule->m_device);
  m_commandList->SetGraphicsRootSignature(rs);
}

/*
*/
void
DX12GPUCommandBuffer::_internalSetIndexBuffer(const SPtr<IndexBuffer>& indexBuff) {
  auto dx12IndexBuff = std::reinterpret_pointer_cast<DX12IndexBuffer>(indexBuff);
  m_commandList->IASetIndexBuffer(&dx12IndexBuff->m_indexBufferView);
}

/*
*/
void
DX12GPUCommandBuffer::_internalDrawIndexed(uint32 indexCountPerInstance, 
                                           uint32 instanceCount, 
                                           uint32 startIndexLocation, 
                                           int32 baseVertexLocation, 
                                           uint32 startIstanceLocation) {
  m_commandList->DrawIndexedInstanced(indexCountPerInstance,
                                      instanceCount,
                                      startIndexLocation,
                                      baseVertexLocation,
                                      startIstanceLocation);
}

}


