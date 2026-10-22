/************************************************************************/
/**
 * @file chProjectDescriptor.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  What a .chproject file says about its project.
 */
/************************************************************************/
#include "chProjectDescriptor.h"

#include "chConfigFile.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chStringUtils.h"

namespace chEngineSDK {

namespace {
constexpr StringView kProjectSection = "Project";
constexpr StringView kModulesSection = "Modules";
constexpr StringView kNameKey = "Name";
constexpr StringView kEngineVersionKey = "EngineVersion";
constexpr StringView kDescriptionKey = "Description";
constexpr StringView kGameModulesKey = "GameModules";
} // namespace

/*
 */
bool
ProjectDescriptor::load(const Path& file)
{
  ConfigFile config;
  if (!config.load(file)) {
    return false;
  }

  const String* nameValue = config.getValue(kProjectSection, kNameKey);
  name = (nameValue && !nameValue->empty()) ? *nameValue : file.getFileName(false);
  if (const String* value = config.getValue(kProjectSection, kEngineVersionKey)) {
    engineVersion = *value;
  }
  if (const String* value = config.getValue(kProjectSection, kDescriptionKey)) {
    description = *value;
  }

  gameModules.clear();
  if (const String* value = config.getValue(kModulesSection, kGameModulesKey)) {
    for (const String& module : StringUtils::splitString(*value, ',')) {
      String trimmed = StringUtils::trim(module);
      if (!trimmed.empty()) {
        gameModules.push_back(std::move(trimmed));
      }
    }
  }
  return true;
}

/*
 */
bool
ProjectDescriptor::save(const Path& file) const
{
  ConfigFile config;
  config.setValue(kProjectSection, kNameKey, name);
  config.setValue(kProjectSection, kEngineVersionKey, engineVersion);
  config.setValue(kProjectSection, kDescriptionKey, description);
  config.setValue(kModulesSection, kGameModulesKey, StringUtils::join(gameModules, ","));
  return config.save(file);
}

#if USING(CH_EDITOR)
/*
 */
bool
ProjectDescriptor::create(const Path& parentDirectory,
                          StringView name,
                          Path& outProjectFile,
                          String& outError)
{
  if (!isValidName(name)) {
    outError = "'" + String(name) + "' cannot be used as a folder name.";
    return false;
  }
  if (parentDirectory.empty()) {
    outError = "Choose the folder the project goes into.";
    return false;
  }

  const String nameText(name);
  const Path projectDirectory = parentDirectory.join(Path(nameText));
  if (FileSystem::exists(projectDirectory)) {
    Vector<Path> files;
    Vector<Path> directories;
    FileSystem::getChildren(projectDirectory, files, directories);
    if (!files.empty() || !directories.empty()) {
      outError = projectDirectory.toString() + " already exists and is not empty.";
      return false;
    }
  }

  if (!FileSystem::createDirectories(projectDirectory.join(Path("Assets"))) ||
      !FileSystem::createDirectories(projectDirectory.join(Path("Config")))) {
    outError = "Could not create " + projectDirectory.toString() + ".";
    return false;
  }

  ProjectDescriptor descriptor;
  descriptor.name = nameText;
  descriptor.engineVersion = CH_ENGINE_VERSION_STRING;
  String fileName = nameText;
  fileName += EnginePaths::PROJECT_EXTENSION;
  outProjectFile = projectDirectory.join(Path(std::move(fileName)));
  if (!descriptor.save(outProjectFile)) {
    outError = "Could not write " + outProjectFile.toString() + ".";
    return false;
  }
  return true;
}

#endif // USING(CH_EDITOR)

/*
 */
Path
ProjectDescriptor::findProjectFile(const Path& directory)
{
  if (!FileSystem::isDirectory(directory)) {
    return Path();
  }

  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(directory, files, directories);
  for (const Path& file : files) {
    if (file.getExtension() == EnginePaths::PROJECT_EXTENSION) {
      return file;
    }
  }
  return Path();
}

/*
 */
bool
ProjectDescriptor::isValidName(StringView name)
{
  if (name.empty() || name == "." || name == ".." || name.back() == '.' ||
      name.back() == ' ') {
    return false;
  }
  for (const ANSICHAR character : name) {
    if (static_cast<uint8>(character) < 32 ||
        StringView("/\\:*?\"<>|").find(character) != StringView::npos) {
      return false;
    }
  }
  return true;
}

} // namespace chEngineSDK
