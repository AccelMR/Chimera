/************************************************************************/
/**
 * @file chEngineConfig.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief Loads the engine, game and editor .ini files.
 */
/************************************************************************/
#include "chEngineConfig.h"

#include "chConsoleVariable.h"
#include "chEnginePaths.h"
#include "chLogger.h"
#include "chPath.h"
#include "chStringUtils.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(EngineConfigLog, All);

namespace {

struct ConfigFileInfo
{
  const ANSICHAR* name;
  const ANSICHAR* engineFolder;
};

constexpr ConfigFileInfo kConfigFiles[] = {
    {.name = "Engine", .engineFolder = "/Engine/Config"},
    {.name = "Game", .engineFolder = "/Engine/Config"},
#if USING(CH_EDITOR)
    {.name = "Editor", .engineFolder = "/Editor/Config"},
#endif // USING(CH_EDITOR)
};

constexpr SIZE_T kConfigFileCount = sizeof(kConfigFiles) / sizeof(kConfigFiles[0]);

struct ConfigState
{
  bool initialized = false;
  Array<ConfigFile, kConfigFileCount> files;
};

NODISCARD ConfigState&
getState()
{
  static ConfigState state;
  return state;
}

NODISCARD Path
getFilePath(const Path& folder, StringView name)
{
  String fileName(name);
  fileName += ".ini";
  return folder.join(Path(fileName));
}

void
sendToConsoleVariables(const ConfigFile& file, ConsoleVariableSource source)
{
  for (const ConfigFile::Section& section : file.getSections()) {
    for (const ConfigFile::Entry& entry : section.entries) {
      if (section.name.empty()) {
        ConsoleVariables::setStartupValue(entry.key, entry.value, source);
        continue;
      }
      String name = section.name;
      name += '.';
      name += entry.key;
      ConsoleVariables::setStartupValue(name, entry.value, source);
    }
  }
}

} // namespace

/*
 */
void
EngineConfig::initialize()
{
  ConfigState& state = getState();
  if (state.initialized) {
    return;
  }
  state.initialized = true;

  // Layer by layer, so a project's Engine.ini goes over the engine's Editor.ini too.
  for (const ConsoleVariableSource source :
       {ConsoleVariableSource::EngineConfig, ConsoleVariableSource::ProjectConfig,
        ConsoleVariableSource::UserConfig}) {
    for (SIZE_T i = 0; i < kConfigFileCount; ++i) {
      const ConfigFileInfo& info = kConfigFiles[i];
      Path folder;
      switch (source) {
        case ConsoleVariableSource::EngineConfig:
          folder = Path(info.engineFolder);
          break;
        case ConsoleVariableSource::ProjectConfig:
          folder = EnginePaths::getProjectConfigDirectory();
          break;
        default:
          folder = EnginePaths::getConfigDirectory();
          break;
      }

      const Path path = getFilePath(folder, info.name);
      ConfigFile layer;
      if (!layer.load(path)) {
        continue;
      }
      CH_LOG_INFO(EngineConfigLog, "Loaded {0}", path);
      sendToConsoleVariables(layer, source);
      state.files[i].merge(layer);
    }
  }

  ConsoleVariables::applyStartupValues();
}

/*
 */
const ConfigFile&
EngineConfig::getFile(StringView name)
{
  ConfigState& state = getState();
  for (SIZE_T i = 0; i < kConfigFileCount; ++i) {
    if (StringUtils::equalsIgnoreCase(kConfigFiles[i].name, name)) {
      return state.files[i];
    }
  }

  static const ConfigFile kEmptyFile;
  return kEmptyFile;
}

/*
 */
bool
EngineConfig::setUserValue(StringView fileName,
                           StringView section,
                           StringView key,
                           StringView value)
{
  const Path path = getFilePath(EnginePaths::getConfigDirectory(), fileName);
  ConfigFile userFile;
  // A missing file starts empty.
  userFile.load(path);
  userFile.setValue(section, key, value);
  if (!userFile.save(path)) {
    CH_LOG_ERROR(EngineConfigLog, "Could not write {0}", path);
    return false;
  }

  ConfigState& state = getState();
  for (SIZE_T i = 0; i < kConfigFileCount; ++i) {
    if (StringUtils::equalsIgnoreCase(kConfigFiles[i].name, fileName)) {
      state.files[i].setValue(section, key, value);
      break;
    }
  }
  return true;
}

} // namespace chEngineSDK
