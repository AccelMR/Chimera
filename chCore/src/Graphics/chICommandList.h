/************************************************************************/
/**
 * @file chICommandList.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for the command list that records the GPU work of a frame.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {

/**
 * Records GPU work. IGraphicsAPI::beginFrame hands out the list of the frame already open,
 * with the bindless heap bound, and endFrame submits it. Called many times per frame, so it
 * takes references and spans: recording a command copies no shared pointer and allocates
 * nothing.
 */
class ICommandList
{
 public:
  ICommandList() = default;
  virtual ~ICommandList() = default;

  /**
   * The targets must already be in the RenderTarget or DepthWrite state.
   */
  virtual void
  beginRendering(const RenderingDesc& desc) = 0;

  virtual void
  endRendering() = 0;

  virtual void
  barrier(Span<const TextureBarrier> textureBarriers) = 0;

  virtual void
  bindPipeline(const IPipeline& pipeline) = 0;

  /**
   * Writes the push constants every shader reads (at most
   * GraphicsLimits::PUSH_CONSTANTS_SIZE bytes, offset included).
   */
  virtual void
  pushConstants(const void* data, uint32 size, uint32 offset = 0) = 0;

  virtual void
  bindVertexBuffer(const IBuffer& buffer, uint32 binding = 0, uint64 offset = 0) = 0;

  virtual void
  bindIndexBuffer(const IBuffer& buffer, IndexType indexType, uint64 offset = 0) = 0;

  virtual void
  draw(uint32 vertexCount,
       uint32 instanceCount = 1,
       uint32 firstVertex = 0,
       uint32 firstInstance = 0) = 0;

  virtual void
  drawIndexed(uint32 indexCount,
              uint32 instanceCount = 1,
              uint32 firstIndex = 0,
              int32 vertexOffset = 0,
              uint32 firstInstance = 0) = 0;

  virtual void
  setViewport(float x, float y,
              float width, float height,
              float minDepth = 0.0f, float maxDepth = 1.0f) = 0;

  virtual void
  setScissor(uint32 x, uint32 y, uint32 width, uint32 height) = 0;
};
} // namespace chEngineSDK
