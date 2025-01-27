/************************************************************************/
/**
 * @file chVulkanGPUCommanderBuffer.h
 * @author AccelMR
 * @date 2025/01/26
 *   Vulkan specific implementation for GPUCommandBuffer.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
  * Includes
  */                                                                     
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include "chGPUCommandBuffer.h"

namespace chEngineSDK {
class VulkanGPUCommandBuffer final : public GPUCommandBuffer {
 public:
  /*
   *   Default constructor
   */
  VulkanGPUCommandBuffer() = default;

  /*
   *   Default destructor
   */
  ~VulkanGPUCommandBuffer();
 
  virtual void
  _init(chGPUDesc::COMMAND_BUFFER_TYPES commandBufferType,
        const SPtr<GPUPipelineState> &pipelineState) override;

  virtual void
  _internalReset(const SPtr<GPUPipelineState>& pipelineState) override;

  virtual void 
  _internalclose() override;
  
  virtual void
  _internalSetPipeLineState(const SPtr<GPUPipelineState>& pipelineState) override;

  virtual void 
  _internalsetGPUBuffer(const SPtr<GPUBuffer>& buff, uint32 slot) override;
  
  virtual void 
  _internalsetGPUBuffers(const Vector<SPtr<GPUBuffer>>& buff, uint32 slot) override;

  virtual void 
  _internalsetRect(const Box2D& rect) override;

  virtual void 
  _internalSetScissorRect(const Box2D& rect) override;

  virtual void 
  _internalsetRenderTarget(const SPtr<Texture>& rt) override;

  virtual void 
  _internalsetRenderTargets(const Vector<SPtr<Texture>>& rts) override;

  virtual void
  _internalclearRenderTarget(const SPtr<Texture>& rt, const LinearColor& color) override;

  virtual void
  _internalclearRenderTargets(const Vector<SPtr<Texture>>& rts, const LinearColor& color) override;

  virtual void
  _internalsetTopology(chGPUDesc::PRIMITIVE_TOPOLOGY_TYPE topology) override;

  virtual void
  _internalsetVertexBuffer(uint32 startSlot, uint32 numViews, const SPtr<VertexBuffer>& vertexBuff) override;

  virtual void
  _internalSetIndexBuffer(const SPtr<IndexBuffer>& indexBuff) override;

  virtual void
  _internalSetBindingBufferGroup(const chGPUDesc::BindingGroup& bindingGroup) override;

  virtual void
  _internaldrawInstanced(uint32 vertexCountPerInstance,
                         uint32 instanceCount,
                         uint32 startVertexLocation,
                         uint32 startInstanceLocation) override;

                         
  virtual void
  _internalDrawIndexed(uint32 indexCountPerInstance,
                       uint32 instanceCount,
                       uint32 startIndexLocation,
                       int32 baseVertexLocation,
                       uint32 startIstanceLocation) override;

  virtual void
  _internalResourceBarrier(const Vector<chGPUDesc::GPUBarrier>& barriers) override;
  
  virtual void
  _internalClearSwapChainTexture(const LinearColor& color) override;

  virtual void
  _internalSetSwapChain() override;

  virtual void
  _internalResourceBarrierSwapChain(const chGPUDesc::GPUBarrier& barriers) override;
  
  VkCommandBuffer m_commandBuffer;
  VkCommandPool m_commandPool;
  VkDevice m_device;
};
} // namespace chEngineSDK
