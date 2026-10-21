/************************************************************************/
/**
 * @file chGpuProfiler.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief Measures how long scopes of a frame take on the GPU.
 */
/************************************************************************/

#include "chGpuProfiler.h"

#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chProfiler.h"

namespace chEngineSDK {
namespace {
constexpr double kNanosecondsPerMillisecond = 1000000.0;
} // namespace

/*
 */
void
GpuProfiler::beginFrame()
{
  const IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  FrameScopes& frame = m_frames[graphicsAPI.getFrameIndex()];

  // A frame that wrote no timestamps (it was skipped, or the GPU cannot measure time)
  // gives none back, and its scopes are dropped.
  const Span<const uint64> timestamps = graphicsAPI.getCompletedTimestamps();
  const double millisecondsPerTick =
      graphicsAPI.getTimestampPeriod() / kNanosecondsPerMillisecond;
  for (uint32 i = 0; i < frame.count && 2 * i + 1 < timestamps.size(); ++i) {
    const uint64 begin = timestamps[2 * i];
    const uint64 end = timestamps[2 * i + 1];
    const uint64 ticks = end > begin ? end - begin : 0;
    Profiler::addGpuTime(frame.scopes[i].name, frame.scopes[i].depth,
                         static_cast<float>(static_cast<double>(ticks) * millisecondsPerTick));
  }

  frame.count = 0;
  m_currentFrame = &frame;
  m_depth = 0;
  m_bRecording = Profiler::isEnabled();
}

/*
 */
uint32
GpuProfiler::beginScope(ICommandList& commandList, StringView name)
{
  if (!m_bRecording || m_currentFrame == nullptr || m_currentFrame->count == MAX_SCOPES) {
    return INVALID_SCOPE;
  }

  const uint32 scope = m_currentFrame->count++;
  m_currentFrame->scopes[scope] = {.name = name, .depth = m_depth};
  ++m_depth;
  commandList.writeTimestamp(2 * scope);
  return scope;
}

/*
 */
void
GpuProfiler::endScope(ICommandList& commandList, uint32 scope)
{
  if (scope == INVALID_SCOPE) {
    return;
  }
  CH_ASSERT(m_depth > 0);
  --m_depth;
  commandList.writeTimestamp(2 * scope + 1);
}

} // namespace chEngineSDK
