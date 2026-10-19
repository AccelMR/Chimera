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
 * What a command list recorded, so the editor can show the cost of each pass. The record
 * functions do nothing in builds without CH_PROFILER, so the counts stay zero there.
 */
struct CommandListStats
{
  /**
   * count is vertices, or indices for indexed draws. Triangles assume triangle lists, so the
   * debug line draws add count / 3 triangles they do not have.
   */
  FORCEINLINE void
  recordDraw(uint32 count, uint32 instanceCount) noexcept
  {
#if USING(CH_PROFILER)
    ++drawCalls;
    vertices += static_cast<uint64>(count) * instanceCount;
    triangles += static_cast<uint64>(count / 3) * instanceCount;
#else
    CH_PARAMETER_UNUSED(count);
    CH_PARAMETER_UNUSED(instanceCount);
#endif // USING(CH_PROFILER)
  }

  FORCEINLINE void
  recordDispatch() noexcept
  {
#if USING(CH_PROFILER)
    ++dispatches;
#endif // USING(CH_PROFILER)
  }

  FORCEINLINE void
  recordPipelineBind() noexcept
  {
#if USING(CH_PROFILER)
    ++pipelineBinds;
#endif // USING(CH_PROFILER)
  }

  FORCEINLINE void
  recordBarriers(SIZE_T count) noexcept
  {
#if USING(CH_PROFILER)
    barriers += static_cast<uint32>(count);
#else
    CH_PARAMETER_UNUSED(count);
#endif // USING(CH_PROFILER)
  }

  FORCEINLINE void
  recordRendering() noexcept
  {
#if USING(CH_PROFILER)
    ++renderings;
#endif // USING(CH_PROFILER)
  }

  NODISCARD constexpr CommandListStats
  operator-(const CommandListStats& other) const noexcept
  {
    return {.drawCalls = drawCalls - other.drawCalls,
            .dispatches = dispatches - other.dispatches,
            .pipelineBinds = pipelineBinds - other.pipelineBinds,
            .barriers = barriers - other.barriers,
            .renderings = renderings - other.renderings,
            .vertices = vertices - other.vertices,
            .triangles = triangles - other.triangles};
  }

  uint32 drawCalls = 0;
  uint32 dispatches = 0;
  uint32 pipelineBinds = 0;
  uint32 barriers = 0;
  uint32 renderings = 0;
  uint64 vertices = 0;
  uint64 triangles = 0;
};

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

  /**
   * Graphics or compute pipeline. A compute pipeline is bound outside a rendering.
   */
  virtual void
  bindPipeline(const IPipeline& pipeline) = 0;

  /**
   * Writes the push constants every shader reads (at most
   * GraphicsLimits::PUSH_CONSTANTS_SIZE bytes, offset included). They reach the kind of
   * pipeline (graphics or compute) bound last, graphics before any bind, so push them after
   * binding the pipeline that reads them.
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

  /**
   * Runs the bound compute pipeline over a grid of thread groups. Not inside a rendering.
   */
  virtual void
  dispatch(uint32 groupCountX, uint32 groupCountY = 1, uint32 groupCountZ = 1) = 0;

  virtual void
  setViewport(float x, float y,
              float width, float height,
              float minDepth = 0.0f, float maxDepth = 1.0f) = 0;

  virtual void
  setScissor(uint32 x, uint32 y, uint32 width, uint32 height) = 0;

  /**
   * Records the GPU time once the commands before it finish, at index (below
   * GraphicsLimits::MAX_TIMESTAMPS_PER_FRAME). Any order works, but by the end of the frame
   * every index below the highest one must be written, or the frame's times are lost;
   * IGraphicsAPI::getCompletedTimestamps gives them back. Not inside a rendering. Does
   * nothing when the GPU cannot measure time.
   */
  virtual void
  writeTimestamp(uint32 index) = 0;

  /**
   * Counts since the list was opened for the frame.
   */
  NODISCARD FORCEINLINE const CommandListStats&
  getStats() const noexcept
  {
    return m_stats;
  }

 protected:
  // Each implementation adds what it records and clears it when the list opens.
  CommandListStats m_stats;
};
} // namespace chEngineSDK
