/************************************************************************/
/**
 * @file chScriptHotReload.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Reads scripts again when their files change.
 */
/************************************************************************/
#include "chScriptHotReload.h"

#include "chAssetManager.h"
#include "chConsoleVariable.h"
#include "chDirectoryWatcher.h"
#include "chEnginePaths.h"
#include "chEventDispatcherManager.h"
#include "chFileSystem.h"
#include "chScriptAsset.h"
#include "chScriptManager.h"
#include "chStringUtils.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(ScriptHotReloadLog, All);

namespace {
ConsoleVariable<bool> g_cvarScriptHotReload(
    "Editor.ScriptHotReload",
    true,
    "Reads scripts again when their files change outside the editor, also during Play.",
    "ScriptHotReload");

ConsoleVariable<float> g_cvarScriptReloadDelay(
    "Editor.ScriptReloadDelay",
    0.1f,
    "Seconds a changed script waits for more changes before it is read: code editors may "
    "write a file in several steps.",
    "ScriptReloadDelay");

ConsoleVariable<float> g_cvarScriptReloadFallbackInterval(
    "Editor.ScriptReloadFallbackInterval",
    1.0f,
    "Seconds between two checks of every script file, only when the system cannot watch "
    "the assets folder.",
    "ScriptReloadFallbackInterval");

struct HotReloadState
{
  DirectoryWatcher watcher;
  // Reused every frame.
  Vector<FileChange> changes;
  // Virtual path of each changed script and when its last change came.
  UnorderedMap<String, double> pendingFiles;
  Vector<String> dueFiles;
  UnorderedMap<UUID, int64> writeTimes;
  HEvent focusEvent;
  double time = 0.0;
  float timeToFallbackCheck = 0.0f;
  bool bCheckAll = false;
};

HotReloadState&
getState()
{
  static HotReloadState state;
  return state;
}

void
reload(HotReloadState& state, const SPtr<IAsset>& asset)
{
  // A failure is logged by the manager, and the script keeps what it ran before.
  static_cast<void>(ScriptManager::instance().reloadScript(asset->as<ScriptAsset>()));
  // Read after the reload, which may have written the UUID line.
  state.writeTimes[asset->getUUID()] = FileSystem::getLastWriteTime(asset->getFilePath());
}

/*
 * Compares the write time of every loaded script with the last one seen. The first time a
 * script is seen it is only remembered.
 */
void
checkAllScripts(HotReloadState& state)
{
  for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
    if (!asset->isTypeOf<ScriptAsset>() || !asset->isLoaded()) {
      continue;
    }
    const int64 writeTime = FileSystem::getLastWriteTime(asset->getFilePath());
    const auto [it, bAdded] = state.writeTimes.try_emplace(asset->getUUID(), writeTime);
    if (!bAdded && it->second != writeTime) {
      reload(state, asset);
    }
  }
}

void
takeChanges(HotReloadState& state)
{
  state.changes.clear();
  if (!state.watcher.poll(state.changes)) {
    state.bCheckAll = true;
  }
  for (const FileChange& change : state.changes) {
    if (change.type != FileChangeType::Removed &&
        StringUtils::equalsIgnoreCase(change.path.getExtension(), ".lua")) {
      const Path file = EnginePaths::getGameAssetDirectory() / change.path;
      state.pendingFiles[file.toString()] = state.time;
    }
  }
}

/*
 * Reads the scripts whose last change is older than the delay.
 */
void
reloadDueFiles(HotReloadState& state)
{
  const double delay = static_cast<double>(g_cvarScriptReloadDelay.get());
  state.dueFiles.clear();
  for (auto it = state.pendingFiles.begin(); it != state.pendingFiles.end();) {
    if (state.time - it->second >= delay) {
      state.dueFiles.push_back(it->first);
      it = state.pendingFiles.erase(it);
    }
    else {
      ++it;
    }
  }
  if (state.dueFiles.empty()) {
    return;
  }
  // Scripts that no scene uses are not loaded and will be read when they are.
  for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
    if (asset->isTypeOf<ScriptAsset>() && asset->isLoaded()) {
      const Path file = asset->getFilePath();
      for (const String& due : state.dueFiles) {
        if (due == file.toString()) {
          reload(state, asset);
          break;
        }
      }
    }
  }
}
} // namespace

/*
 */
void
ScriptHotReload::initialize()
{
  HotReloadState& state = getState();
  if (!state.watcher.start(EnginePaths::getGameAssetDirectory(), true)) {
    CH_LOG_WARNING(ScriptHotReloadLog,
                   "Script files are checked every {0} s instead of being watched.",
                   g_cvarScriptReloadFallbackInterval.get());
  }
  state.focusEvent = EventDispatcherManager::instance().OnFocusChanged.connect(
      [](const bool bFocused) {
        if (bFocused) {
          getState().bCheckAll = true;
        }
      });
}

/*
 */
void
ScriptHotReload::shutDown()
{
  HotReloadState& state = getState();
  state.focusEvent.disconnect();
  state.watcher.stop();
  state.pendingFiles.clear();
  state.writeTimes.clear();
}

/*
 */
void
ScriptHotReload::update(const float deltaTime)
{
  HotReloadState& state = getState();
  state.time += static_cast<double>(deltaTime);
  if (state.watcher.isWatching()) {
    takeChanges(state);
  }
  else {
    state.timeToFallbackCheck -= deltaTime;
    if (state.timeToFallbackCheck <= 0.0f) {
      state.timeToFallbackCheck = g_cvarScriptReloadFallbackInterval.get();
      state.bCheckAll = true;
    }
  }

  if (!g_cvarScriptHotReload.get() || !ScriptManager::isStarted() ||
      !AssetManager::isStarted()) {
    state.pendingFiles.clear();
    state.bCheckAll = false;
    return;
  }
  if (state.bCheckAll) {
    state.bCheckAll = false;
    checkAllScripts(state);
  }
  if (!state.pendingFiles.empty()) {
    reloadDueFiles(state);
  }
}

} // namespace chEngineSDK
