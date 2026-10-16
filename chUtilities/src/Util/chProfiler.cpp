/************************************************************************/
/**
 * @file chProfiler.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief Measures how long named scopes of the main thread take each frame.
 */
/************************************************************************/

#include "chProfiler.h"

#if USING(CH_PROFILER)

#include <chrono>

#include "chConsoleVariable.h"

namespace chEngineSDK {
namespace {
ConsoleVariable<float> g_cvarProfilerUpdateInterval(
    "Profiler.UpdateInterval",
    0.5f,
    "Seconds the profiler averages its scope times over before showing them.");

constexpr uint32 kNoParent = ~0u;
constexpr double kNanosecondsPerMillisecond = 1000000.0;
constexpr double kNanosecondsPerSecond = 1000000000.0;

struct ScopeRecord
{
  StringView name;
  uint32 parent = kNoParent;
  int64 start = 0;
  int64 end = 0;
};

/**
 * A scope of the tree being averaged. Node 0 is the frame.
 */
struct AggregateNode
{
  StringView name;
  uint32 firstChild = kNoParent;
  uint32 lastChild = kNoParent;
  uint32 nextSibling = kNoParent;
  uint32 calls = 0;
  int64 nanoseconds = 0;
};

/**
 * A GPU time being averaged. They arrive in the order they ran, parents first, so the list
 * keeps that order and depth is enough to nest them.
 */
struct GpuNode
{
  StringView name;
  uint32 depth = 0;
  uint32 calls = 0;
  double milliseconds = 0.0;
};

struct ProfilerData
{
  ProfilerData()
  {
    nodes.reserve(Profiler::MAX_SCOPES_PER_FRAME);
    published.reserve(Profiler::MAX_SCOPES_PER_FRAME);
    gpuNodes.reserve(Profiler::MAX_GPU_SCOPES);
    publishedGpu.reserve(Profiler::MAX_GPU_SCOPES);
    resetInterval(0);
  }

  void
  resetInterval(int64 now)
  {
    nodes.clear();
    nodes.push_back({.name = "Frame"});
    gpuNodes.clear();
    intervalStart = now;
    intervalFrames = 0;
  }

  bool bEnabled = false;
  bool bInFrame = false;
  int64 frameStart = 0;

  Array<ScopeRecord, Profiler::MAX_SCOPES_PER_FRAME> records{};
  // The aggregate node each record of the frame added to.
  Array<uint32, Profiler::MAX_SCOPES_PER_FRAME> recordNodes{};
  uint32 recordCount = 0;
  Array<uint32, Profiler::MAX_SCOPE_DEPTH> openScopes{};
  uint32 openCount = 0;

  Vector<AggregateNode> nodes;
  int64 intervalStart = 0;
  uint32 intervalFrames = 0;

  Vector<ProfilerScopeStats> published;
  float averageFrameMilliseconds = 0.0f;

  Vector<GpuNode> gpuNodes;
  Vector<ProfilerScopeStats> publishedGpu;
  float averageGpuFrameMilliseconds = 0.0f;

  Array<float, Profiler::FRAME_HISTORY_SIZE> history{};
  uint32 historyNext = 0;
};

ProfilerData&
getData()
{
  static ProfilerData data;
  return data;
}

thread_local bool t_bMainThread = false;

int64
now()
{
  using namespace std::chrono;
  return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

uint32
findOrAddChild(ProfilerData& data, uint32 parent, StringView name)
{
  uint32 child = data.nodes[parent].firstChild;
  while (child != kNoParent) {
    if (data.nodes[child].name == name) {
      return child;
    }
    child = data.nodes[child].nextSibling;
  }

  // A full tree adds the time to the parent instead of losing it.
  if (data.nodes.size() >= Profiler::MAX_SCOPES_PER_FRAME) {
    return parent;
  }

  const uint32 index = static_cast<uint32>(data.nodes.size());
  data.nodes.push_back({.name = name});
  AggregateNode& parentNode = data.nodes[parent];
  if (parentNode.lastChild == kNoParent) {
    parentNode.firstChild = index;
  }
  else {
    data.nodes[parentNode.lastChild].nextSibling = index;
  }
  parentNode.lastChild = index;
  return index;
}

void
publishNode(ProfilerData& data, uint32 node, uint32 depth)
{
  const AggregateNode& aggregate = data.nodes[node];
  const float frames = static_cast<float>(data.intervalFrames);
  data.published.push_back(
      {.name = aggregate.name,
       .depth = depth,
       .callsPerFrame = static_cast<float>(aggregate.calls) / frames,
       .milliseconds = static_cast<float>(static_cast<double>(aggregate.nanoseconds) /
                                          kNanosecondsPerMillisecond) /
                       frames});

  for (uint32 child = aggregate.firstChild; child != kNoParent;
       child = data.nodes[child].nextSibling) {
    publishNode(data, child, depth + 1);
  }
}
} // namespace

/*
 */
void
Profiler::beginFrame()
{
  ProfilerData& data = getData();
  t_bMainThread = true;
  data.bInFrame = data.bEnabled;
  data.recordCount = 0;
  data.openCount = 0;
  data.frameStart = now();
}

/*
 */
void
Profiler::endFrame()
{
  ProfilerData& data = getData();
  if (!data.bInFrame) {
    return;
  }
  data.bInFrame = false;
  CH_ASSERT(data.openCount == 0 && "A profile scope is still open at the end of the frame.");

  const int64 frameEnd = now();
  const int64 frameNanoseconds = frameEnd - data.frameStart;
  data.history[data.historyNext] =
      static_cast<float>(static_cast<double>(frameNanoseconds) / kNanosecondsPerMillisecond);
  data.historyNext = (data.historyNext + 1) % FRAME_HISTORY_SIZE;

  data.nodes[0].calls += 1;
  data.nodes[0].nanoseconds += frameNanoseconds;
  // A parent is recorded before its children, so its node is always known.
  for (uint32 i = 0; i < data.recordCount; ++i) {
    const ScopeRecord& record = data.records[i];
    const uint32 parentNode = record.parent == kNoParent ? 0 : data.recordNodes[record.parent];
    const uint32 node = findOrAddChild(data, parentNode, record.name);
    data.recordNodes[i] = node;
    data.nodes[node].calls += 1;
    data.nodes[node].nanoseconds += record.end - record.start;
  }
  ++data.intervalFrames;

  const double intervalSeconds =
      static_cast<double>(frameEnd - data.intervalStart) / kNanosecondsPerSecond;
  if (intervalSeconds < static_cast<double>(g_cvarProfilerUpdateInterval.get())) {
    return;
  }

  data.published.clear();
  publishNode(data, 0, 0);
  data.averageFrameMilliseconds = data.published.front().milliseconds;

  const float frames = static_cast<float>(data.intervalFrames);
  data.publishedGpu.clear();
  data.averageGpuFrameMilliseconds = 0.0f;
  for (const GpuNode& node : data.gpuNodes) {
    const float milliseconds = static_cast<float>(node.milliseconds) / frames;
    data.publishedGpu.push_back({.name = node.name,
                                 .depth = node.depth,
                                 .callsPerFrame = static_cast<float>(node.calls) / frames,
                                 .milliseconds = milliseconds});
    if (node.depth == 0) {
      data.averageGpuFrameMilliseconds += milliseconds;
    }
  }

  data.resetInterval(frameEnd);
}

/*
 */
void
Profiler::addGpuTime(StringView name, uint32 depth, float milliseconds)
{
  ProfilerData& data = getData();
  if (!t_bMainThread || !data.bInFrame) {
    return;
  }

  for (GpuNode& node : data.gpuNodes) {
    if (node.depth == depth && node.name == name) {
      node.calls += 1;
      node.milliseconds += static_cast<double>(milliseconds);
      return;
    }
  }
  if (data.gpuNodes.size() < MAX_GPU_SCOPES) {
    data.gpuNodes.push_back({.name = name,
                             .depth = depth,
                             .calls = 1,
                             .milliseconds = static_cast<double>(milliseconds)});
  }
}

/*
 */
Span<const ProfilerScopeStats>
Profiler::getGpuScopeStats()
{
  return getData().publishedGpu;
}

/*
 */
float
Profiler::getAverageGpuFrameMilliseconds()
{
  return getData().averageGpuFrameMilliseconds;
}

/*
 */
void
Profiler::setEnabled(bool bEnabled)
{
  ProfilerData& data = getData();
  if (data.bEnabled == bEnabled) {
    return;
  }
  data.bEnabled = bEnabled;
  // The interval starts again, so time spent disabled is not averaged in.
  data.resetInterval(now());
}

/*
 */
bool
Profiler::isEnabled()
{
  return getData().bEnabled;
}

/*
 */
uint32
Profiler::beginScope(StringView name)
{
  ProfilerData& data = getData();
  if (!t_bMainThread || !data.bInFrame || data.recordCount == MAX_SCOPES_PER_FRAME ||
      data.openCount == MAX_SCOPE_DEPTH) {
    return INVALID_SCOPE;
  }

  const uint32 index = data.recordCount++;
  data.records[index] = {
      .name = name,
      .parent = data.openCount > 0 ? data.openScopes[data.openCount - 1] : kNoParent,
      .start = now(),
      .end = 0};
  data.openScopes[data.openCount++] = index;
  return index;
}

/*
 */
void
Profiler::endScope(uint32 scope)
{
  ProfilerData& data = getData();
  if (!data.bInFrame || data.openCount == 0) {
    return;
  }
  CH_ASSERT(data.openScopes[data.openCount - 1] == scope &&
            "Profile scopes must end in the reverse order they began.");
  data.records[scope].end = now();
  --data.openCount;
}

/*
 */
Span<const ProfilerScopeStats>
Profiler::getScopeStats()
{
  return getData().published;
}

/*
 */
float
Profiler::getAverageFrameMilliseconds()
{
  return getData().averageFrameMilliseconds;
}

/*
 */
Span<const float>
Profiler::getFrameHistory()
{
  return getData().history;
}

/*
 */
uint32
Profiler::getFrameHistoryOffset()
{
  return getData().historyNext;
}

} // namespace chEngineSDK

#endif // USING(CH_PROFILER)
