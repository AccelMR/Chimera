/************************************************************************/
/**
 * @file chGPUCommandBuffer.h
 * @author AccelMR
 * @date 2022/09/28
 * @brief GPU Command buffer. Used to send instructions to the gpu to be 
 *        executed at the render pass.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include <chBox2D.h>

#include "chPrerequisitesGraphics.h"

namespace chEngineSDK {
class CH_CORE_EXPORT GPUCommandBuffer
{
 public:
 /*
  *   Default constructor
  */
  GPUCommandBuffer() = default;

 /*
  *   Default destructor
  */
  virtual ~GPUCommandBuffer() = default;

  //TODO: probably we must rename this as setPipeline or something since this also
  //sets the root signature, or registers for shared memory from GPU
  //Binding groups are in the shader since the shader signs which are their bindings,
  //but those shaders are also kept in the pipeline, so it make sense to me to just set
  //the root signature here that you also have the pipeline and its shaders.
  void
  reset(const SPtr<GPUPipelineState>& pipelineState);

  void
  setPipeLineState(const SPtr<GPUPipelineState>& pipelineState);

  void 
  close();

  void 
  setGPUBuffer(const SPtr<GPUBuffer>& buff, uint32 slot);
  
  void 
  setGPUBuffers(const Vector<SPtr<GPUBuffer>>& buff, uint32 slot);

  void 
  setRect(const Box2D& rect);

  void 
  setScissorRect(const Box2D& rect);

  void 
  setRenderTarget(const SPtr<Texture>& rt);

  void 
  setRenderTargets(const Vector<SPtr<Texture>>& rts);

  void
  clearRenderTarget(const SPtr<Texture>& rt, const LinearColor& color);

  void
  clearRenderTargets(const Vector<SPtr<Texture>>& rts, const LinearColor& color);

  void
  setTopology(chGPUDesc::PRIMITIVE_TOPOLOGY_TYPE topology);

  void
  setVertexBuffer(uint32 startSlot, uint32 numViews, const SPtr<VertexBuffer>& vertexBuff);

  void
  setIndexBuffer(const SPtr<IndexBuffer>& indexBuff);

  void
  setBindingBufferGroup(const chGPUDesc::BindingGroup& bindingGroup);

  void
  drawInstanced(uint32 vertexCountPerInstance,
                uint32 instanceCount,
                uint32 startVertexLocation,
                uint32 startInstanceLocation);


  void
  drawIndexed(uint32 indexCountPerInstance,
              uint32 instanceCount,
              uint32 startIndexLocation,
              int32 baseVertexLocation,
              uint32 startIstanceLocation);

  void 
  resourceBarrier(const Vector<chGPUDesc::GPUBarrier>& barriers);

  void
  resourceBarrierSwapChain(const chGPUDesc::GPUBarrier& barriers);

  void 
  setSwapChainTexture(const SPtr<Texture>& rt);

  void
  clearSwapChainTexture(const LinearColor& color);

  bool
  present(int32 syncInterval, int32 flags);

 protected:
 friend class GraphicsModule;
 
  virtual void
  _init(chGPUDesc::COMMAND_BUFFER_TYPES commandBufferType,
        const SPtr<GPUPipelineState> &pipelineState) = 0;

  virtual void
  _internalReset(const SPtr<GPUPipelineState>& pipelineState) = 0;

  virtual void 
  _internalclose() = 0;
  
  virtual void
  _internalSetPipeLineState(const SPtr<GPUPipelineState>& pipelineState) = 0;

  virtual void 
  _internalSetGPUBuffer(const SPtr<GPUBuffer>& buff, uint32 slot) = 0;
  
  virtual void 
  _internalSetGPUBuffers(const Vector<SPtr<GPUBuffer>>& buff, uint32 slot) = 0;

  virtual void 
  _internalSetRect(const Box2D& rect) = 0;

  virtual void 
  _internalSetScissorRect(const Box2D& rect) = 0;

  virtual void 
  _internalSetRenderTarget(const SPtr<Texture>& rt) = 0;

  virtual void 
  _internalSetRenderTargets(const Vector<SPtr<Texture>>& rts) = 0;

  virtual void
  _internalClearRenderTarget(const SPtr<Texture>& rt, const LinearColor& color) = 0;

  virtual void
  _internalClearRenderTargets(const Vector<SPtr<Texture>>& rts, const LinearColor& color) = 0;

  virtual void
  _internalSetTopology(chGPUDesc::PRIMITIVE_TOPOLOGY_TYPE topology) = 0;

  virtual void
  _internalSetVertexBuffer(uint32 startSlot, uint32 numViews, const SPtr<VertexBuffer>& vertexBuff) = 0;

  virtual void
  _internalSetIndexBuffer(const SPtr<IndexBuffer>& indexBuff) = 0;

  virtual void
  _internalSetBindingBufferGroup(const chGPUDesc::BindingGroup& bindingGroup) = 0;

  virtual void
  _internalDrawInstanced(uint32 vertexCountPerInstance,
                         uint32 instanceCount,
                         uint32 startVertexLocation,
                         uint32 startInstanceLocation) = 0;

  virtual void
  _internalDrawIndexed(uint32 indexCountPerInstance,
                       uint32 instanceCount,
                       uint32 startIndexLocation,
                       int32 baseVertexLocation,
                       uint32 startIstanceLocation) = 0;

  virtual void
  _internalResourceBarrier(const Vector<chGPUDesc::GPUBarrier>& barriers) = 0;
  
  virtual void
  _internalClearSwapChainTexture(const LinearColor& color) = 0;

  virtual void
  _internalSetSwapChainTexture(const SPtr<Texture>& rt) = 0;

  virtual void
  _internalResourceBarrierSwapChain(const chGPUDesc::GPUBarrier& barriers) = 0;

  virtual bool
  _internalPresent(int32 syncInterval, int32 flags) = 0;
};
}

