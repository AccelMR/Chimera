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
  _internalSetGPUBuffer(const SPtr<GPUBuffer>& buff, uint32 slot) override;
  
  virtual void 
  _internalSetGPUBuffers(const Vector<SPtr<GPUBuffer>>& buff, uint32 slot) override;

  virtual void 
  _internalSetRect(const Box2D& rect) override;

  virtual void 
  _internalSetScissorRect(const Box2D& rect) override;

  virtual void 
  _internalSetRenderTarget(const SPtr<Texture>& rt) override;

  virtual void 
  _internalSetRenderTargets(const Vector<SPtr<Texture>>& rts) override;

  virtual void
  _internalClearRenderTarget(const SPtr<Texture>& rt, const LinearColor& color) override;

  virtual void
  _internalClearRenderTargets(const Vector<SPtr<Texture>>& rts, const LinearColor& color) override;

  virtual void
  _internalSetTopology(chGPUDesc::PRIMITIVE_TOPOLOGY_TYPE topology) override;

  virtual void
  _internalSetVertexBuffer(uint32 startSlot, uint32 numViews, const SPtr<VertexBuffer>& vertexBuff) override;

  virtual void
  _internalSetIndexBuffer(const SPtr<IndexBuffer>& indexBuff) override;

  virtual void
  _internalSetBindingBufferGroup(const chGPUDesc::BindingGroup& bindingGroup) override;

  virtual void
  _internalDrawInstanced(uint32 vertexCountPerInstance,
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
  _internalResourceBarrierSwapChain(const chGPUDesc::GPUBarrier& barriers) override;

  virtual void
  _internalSetSwapChainTexture(const SPtr<Texture>& rt) override;

  virtual bool
  _internalPresent(int32 syncInterval, int32 flags) override;

  VkCommandBuffer m_commandBuffer;
  VkCommandPool m_commandPool;
  VkDevice m_device;
};
} // namespace chEngineSDK
