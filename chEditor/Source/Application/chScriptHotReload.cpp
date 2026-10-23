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
#include "chEventDispatcherManager.h"
#include "chFileSystem.h"
#include "chScriptAsset.h"
#include "chScriptManager.h"

namespace chEngineSDK {

namespace {
ConsoleVariable<float> g_cvarScriptReloadInterval(
    "Editor.ScriptReloadInterval",
    0.5f,
    "Seconds between two checks for script files saved outside the editor, which are then "
    "read again, also during Play. 0 turns it off.",
    "ScriptReloadInterval");

struct HotReloadState
{
  UnorderedMap<UUID, int64> writeTimes;
  HEvent focusEvent;
  float timeToCheck = 0.0f;
  bool bCheckNow = false;
};

HotReloadState&
getState()
{
  static HotReloadState state;
  return state;
}

void
checkScripts(HotReloadState& state)
{
  ScriptManager& scriptManager = ScriptManager::instance();
  for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
    if (!asset->isTypeOf<ScriptAsset>() || !asset->isLoaded()) {
      continue;
    }
    const int64 writeTime = FileSystem::getLastWriteTime(asset->getFilePath());
    const auto [it, bAdded] = state.writeTimes.try_emplace(asset->getUUID(), writeTime);
    if (bAdded || it->second == writeTime) {
      continue;
    }
    it->second = writeTime;
    // A failure is logged by the manager, and the script keeps what it ran before.
    static_cast<void>(scriptManager.reloadScript(asset->as<ScriptAsset>()));
    // Reading the file again may have written its UUID line.
    it->second = FileSystem::getLastWriteTime(asset->getFilePath());
  }
}
} // namespace

/*
 */
void
ScriptHotReload::initialize()
{
  HotReloadState& state = getState();
  state.focusEvent = EventDispatcherManager::instance().OnFocusChanged.connect(
      [](const bool bFocused) {
        if (bFocused) {
          getState().bCheckNow = true;
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
  state.writeTimes.clear();
}

/*
 */
void
ScriptHotReload::update(const float deltaTime)
{
  HotReloadState& state = getState();
  const float interval = g_cvarScriptReloadInterval.get();
  if (interval <= 0.0f) {
    return;
  }
  state.timeToCheck -= deltaTime;
  if ((!state.bCheckNow && state.timeToCheck > 0.0f) || !ScriptManager::isStarted() ||
      !AssetManager::isStarted()) {
    return;
  }
  state.bCheckNow = false;
  state.timeToCheck = interval;
  checkScripts(state);
}

} // namespace chEngineSDK
