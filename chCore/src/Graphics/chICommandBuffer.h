/************************************************************************/
/**
 * @file chICommandBuffer.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for the command buffer. This is the base class for all command
 * buffers. It is used to record commands and submit them to the command queue.
 * It is also used to reset the command buffer and free it.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
enum class CommandBufferState: uint8{
  Initial,
  Recording,
  Executable,
  Pending,
  Invalid
};

class CH_CORE_EXPORT ICommandBuffer {
 public:
  ICommandBuffer() = default;
  virtual ~ICommandBuffer() = default;

  virtual void 
  begin() = 0;

  virtual void 
  end() = 0;

  virtual void
  beginRenderPass(const RenderPassBeginInfo& beginInfo) = 0;

  virtual void
  endRenderPass() = 0;

  virtual void 
  bindPipeline(SPtr<IPipeline> pipeline) = 0;

  virtual void 
  bindVertexBuffer(SPtr<IBuffer> buffer, 
                   uint32 binding = 0, 
                   uint64 offset = 0) = 0;

  virtual void 
  bindIndexBuffer(SPtr<IBuffer> buffer, 
                  IndexType indexType, 
                  uint32 offset = 0) = 0;

  virtual void 
  draw(uint32 vertexCount, 
       uint32 instanceCount = 1, 
       uint32 firstVertex = 0, 
       uint32 firstInstance = 0) = 0;

  virtual void 
  drawIndexed(uint32 indexCount, uint32 instanceCount = 1,
              uint32 firstIndex = 0, int32 vertexOffset = 0, 
              uint32 firstInstance = 0) = 0;

  virtual void 
  setViewport(float x, float y, 
              float width, float height,
              float minDepth = 0.0f, float maxDepth = 1.0f) = 0;

  virtual void 
  setScissor(uint32 x, uint32 y, uint32 width, uint32 height) = 0;

  NODISCARD virtual 
  CommandBufferState getState() const = 0;
};
} // namespace chEngineSDK