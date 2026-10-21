/************************************************************************/
/**
 * @file chGpuProfiler.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief Measures how long scopes of a frame take on the GPU.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"

namespace chEngineSDK {

class ICommandList;

/**
 * Exists so render passes can be timed on the GPU: a scope writes a timestamp where it
 * begins and where it ends, and once the GPU finished that frame (read in a later frame
 * that reuses its slot, so nothing waits) the times go to the Profiler, which averages and
 * shows them next to the CPU ones. Records nothing while the Profiler is disabled.
 *
 * Scope names are kept until the times come back, so they must be string literals.
 */
class CH_CORE_EXPORT GpuProfiler
{
 public:
  /**
   * Hands the times of the frame that last used this slot to the Profiler. Call once per
   * frame, after IGraphicsAPI::beginFrame and before any scope.
   */
  void
  beginFrame();

  /**
   * Returns INVALID_SCOPE when nothing is measured; pass the result to endScope. Not
   * inside a rendering.
   */
  NODISCARD uint32
  beginScope(ICommandList& commandList, StringView name);

  void
  endScope(ICommandList& commandList, uint32 scope);

  // Each scope writes two timestamps.
  static constexpr uint32 MAX_SCOPES = GraphicsLimits::MAX_TIMESTAMPS_PER_FRAME / 2;
  static constexpr uint32 INVALID_SCOPE = ~0u;

 private:
  struct Scope
  {
    StringView name;
    uint32 depth = 0;
  };

  struct FrameScopes
  {
    Array<Scope, MAX_SCOPES> scopes{};
    uint32 count = 0;
  };

  Array<FrameScopes, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_frames{};
  FrameScopes* m_currentFrame = nullptr;
  uint32 m_depth = 0;
  bool m_bRecording = false;
};

} // namespace chEngineSDK
