/************************************************************************/
/**
 * @file chProfiler.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief Measures how long named scopes of the main thread take each frame.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {

/**
 * One scope of the profiler's averaged tree. The first one is the whole frame.
 */
struct ProfilerScopeStats
{
  StringView name;
  // The scopes are listed parent first, so the children of a scope follow it with a
  // bigger depth.
  uint32 depth = 0;
  float callsPerFrame = 0.0f;
  // Its children included.
  float milliseconds = 0.0f;
};

/**
 * Exists so code can measure itself with CH_PROFILE_SCOPE("Name") and the editor can show
 * where the main thread spends each frame. Scopes nest into a tree, and scopes with the
 * same name under the same parent add up. Times are averaged over Profiler.UpdateInterval
 * seconds, so the numbers stay readable, and the times of the last frames are kept for a
 * graph.
 *
 * Only the thread that calls beginFrame is measured; scopes on other threads do nothing,
 * and so does every scope while the profiler is disabled. A scope must start and end inside
 * the same frame. The storage is fixed, so measuring allocates nothing. Scope names are
 * kept after the frame, so they must be string literals.
 *
 * Built without CH_PROFILER (CMake option CH_ENABLE_PROFILER off, for shipping) every
 * function is an empty inline and CH_PROFILE_SCOPE expands to nothing, so callers need no
 * checks of their own and nothing is left in the build.
 */
class Profiler
{
 public:
#if USING(CH_PROFILER)
  CH_UTILITY_EXPORT static void
  beginFrame();

  CH_UTILITY_EXPORT static void
  endFrame();

  CH_UTILITY_EXPORT static void
  setEnabled(bool bEnabled);

  NODISCARD CH_UTILITY_EXPORT static bool
  isEnabled();

  /**
   * Returns INVALID_SCOPE when nothing is measured; pass the result to endScope.
   */
  NODISCARD CH_UTILITY_EXPORT static uint32
  beginScope(StringView name);

  CH_UTILITY_EXPORT static void
  endScope(uint32 scope);

  /**
   * The tree averaged over the last finished interval.
   */
  NODISCARD CH_UTILITY_EXPORT static Span<const ProfilerScopeStats>
  getScopeStats();

  NODISCARD CH_UTILITY_EXPORT static float
  getAverageFrameMilliseconds();

  /**
   * Milliseconds of the last FRAME_HISTORY_SIZE frames, a ring that starts at
   * getFrameHistoryOffset() (the oldest).
   */
  NODISCARD CH_UTILITY_EXPORT static Span<const float>
  getFrameHistory();

  NODISCARD CH_UTILITY_EXPORT static uint32
  getFrameHistoryOffset();
#else
  static void
  beginFrame()
  {}

  static void
  endFrame()
  {}

  static void
  setEnabled(bool bEnabled)
  {
    CH_PARAMETER_UNUSED(bEnabled);
  }

  NODISCARD static bool
  isEnabled()
  {
    return false;
  }

  NODISCARD static uint32
  beginScope(StringView name)
  {
    CH_PARAMETER_UNUSED(name);
    return INVALID_SCOPE;
  }

  static void
  endScope(uint32 scope)
  {
    CH_PARAMETER_UNUSED(scope);
  }

  NODISCARD static Span<const ProfilerScopeStats>
  getScopeStats()
  {
    return {};
  }

  NODISCARD static float
  getAverageFrameMilliseconds()
  {
    return 0.0f;
  }

  NODISCARD static Span<const float>
  getFrameHistory()
  {
    return {};
  }

  NODISCARD static uint32
  getFrameHistoryOffset()
  {
    return 0;
  }
#endif // USING(CH_PROFILER)

  static constexpr uint32 MAX_SCOPES_PER_FRAME = 1024;
  static constexpr uint32 MAX_SCOPE_DEPTH = 32;
  static constexpr uint32 FRAME_HISTORY_SIZE = 240;
  static constexpr uint32 INVALID_SCOPE = ~0u;
};

/**
 * Measures from its construction to its destruction. Use it through CH_PROFILE_SCOPE.
 */
class ProfileScope
{
 public:
  explicit ProfileScope(StringView name)
    : m_scope(Profiler::beginScope(name))
  {}

  ~ProfileScope()
  {
    if (m_scope != Profiler::INVALID_SCOPE) {
      Profiler::endScope(m_scope);
    }
  }

  ProfileScope(const ProfileScope&) = delete;

  ProfileScope&
  operator=(const ProfileScope&) = delete;

 private:
  uint32 m_scope;
};

} // namespace chEngineSDK

#if USING(CH_PROFILER)
# define CH_PROFILE_CONCAT_INNER(a, b) a##b
# define CH_PROFILE_CONCAT(a, b) CH_PROFILE_CONCAT_INNER(a, b)

/**
 * Measures the rest of the enclosing block under name (a string literal).
 */
# define CH_PROFILE_SCOPE(name)                                                               \
   ::chEngineSDK::ProfileScope CH_PROFILE_CONCAT(chProfileScope, __LINE__)(name)
#else
# define CH_PROFILE_SCOPE(name)
#endif // USING(CH_PROFILER)
