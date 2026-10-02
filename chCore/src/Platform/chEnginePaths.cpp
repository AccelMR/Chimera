/************************************************************************/
/**
 * @file chEnginePaths.cpp
 * @author AccelMR
 * @date 2025/07/15
 * @brief
 *          This file contains the implementation of the EnginePaths class,
 */
/************************************************************************/
#include "chEnginePaths.h"

#include "chCommandParser.h"
#include "chException.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chPath.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(EnginePathsLog, All);

namespace {
const ANSICHAR* const kEngineContentFolder = "chCore/Content";
const ANSICHAR* const kEditorContentFolder = "chEditor/Content";
const ANSICHAR* const kDefaultProjectFolder = "Projects/Sandbox";
const ANSICHAR* const kDefaultProjectName = "Sandbox";
const ANSICHAR* const kProjectExtension = ".chproject";

struct PathState
{
  bool initialized = false;
  Path executableDirectory;
  Path engineRoot;
  Path projectRoot;
};

PathState&
pathState()
{
  static PathState state;
  return state;
}

Path
findEngineRoot(const Path& startDirectory)
{
  Path directory = startDirectory;
  while (!directory.empty()) {
    if (FileSystem::isDirectory(directory.join(Path(kEngineContentFolder)))) {
      return directory;
    }

    const Path parent = directory.getDirectory();
    if (parent == directory) {
      break;
    }
    directory = parent;
  }
  return Path();
}

bool
hasProjectFile(const Path& directory)
{
  if (!FileSystem::isDirectory(directory)) {
    return false;
  }

  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(directory, files, directories);
  for (const Path& file : files) {
    if (file.getExtension() == kProjectExtension) {
      return true;
    }
  }
  return false;
}

Path
findProjectRoot(const Path& engineRoot, const Path& executableDirectory)
{
  String requestedProject;
  if (CommandParser::isStarted()) {
    requestedProject = CommandParser::instance().getParam("project");
  }

  if (!requestedProject.empty()) {
    Path project = FileSystem::absolutePath(Path(requestedProject));
    if (FileSystem::isFile(project) && project.getExtension() == kProjectExtension) {
      project = project.getDirectory();
    }

    if (!hasProjectFile(project)) {
      CH_EXCEPT(InvalidArgumentException,
                "-project must be a folder with a " + String(kProjectExtension) +
                    " file, or the file itself: " + project.toString());
    }
    return project;
  }

#if USING(CH_EDITOR)
  CH_PARAMETER_UNUSED(executableDirectory);

  const Path project = engineRoot.join(Path(kDefaultProjectFolder));
  if (!hasProjectFile(project)) {
    CH_LOG_INFO(EnginePathsLog, "Creating the default project in {0}", project);
    const Path projectFile =
        project.join(Path(String(kDefaultProjectName) + kProjectExtension));
    SPtr<DataStream> file = FileSystem::createAndOpenFile(projectFile);
    if (!file) {
      CH_EXCEPT(InternalErrorException,
                "Could not create the default project: " + projectFile.toString());
    }
    file->close();
  }
  return project;
#else
  CH_PARAMETER_UNUSED(engineRoot);
  return executableDirectory;
#endif
}
} // namespace

void
EnginePaths::initialize()
{
  PathState& state = pathState();
  if (state.initialized) {
    return;
  }

  state.executableDirectory = FileSystem::getExecutableDirectory();
  state.engineRoot = findEngineRoot(state.executableDirectory);
  if (state.engineRoot.empty()) {
    CH_EXCEPT(InternalErrorException,
              "Could not find " + String(kEngineContentFolder) + " in " +
                  state.executableDirectory.toString() + " or any folder above it.");
  }

  state.projectRoot = findProjectRoot(state.engineRoot, state.executableDirectory);
  FileSystem::setBaseDirectory(state.projectRoot);

  const Path assetsDirectory = state.projectRoot.join(Path("Assets"));
  const Path savedDirectory = state.projectRoot.join(Path("Saved"));
  FileSystem::createDirectories(assetsDirectory);
  FileSystem::createDirectories(savedDirectory.join(Path("Logs")));
  FileSystem::createDirectories(savedDirectory.join(Path("Config")));

  FileSystem::mount("Engine", state.engineRoot.join(Path(kEngineContentFolder)));
#if USING(CH_EDITOR)
  FileSystem::mount("Editor", state.engineRoot.join(Path(kEditorContentFolder)));
#endif
  FileSystem::mount("Game", assetsDirectory, 0, true);
  FileSystem::mount("Saved", savedDirectory, 0, true);

  state.initialized = true;

  CH_LOG_INFO(EnginePathsLog, "Engine root: {0}", state.engineRoot);
  CH_LOG_INFO(EnginePathsLog, "Project: {0}", state.projectRoot);
}

Path
EnginePaths::getEngineRootDirectory()
{
  initialize();
  return pathState().engineRoot;
}

Path
EnginePaths::getProjectDirectory()
{
  initialize();
  return pathState().projectRoot;
}

Path
EnginePaths::getPluginDirectory()
{
  initialize();
  return pathState().executableDirectory;
}

Path
EnginePaths::getCodecDirectory()
{
  return getPluginDirectory().join(Path("Codecs"));
}

Path
EnginePaths::getGameAssetDirectory()
{
  initialize();
  return Path("/Game");
}

Path
EnginePaths::getAbsoluteGameAssetDirectory()
{
  return getProjectDirectory().join(Path("Assets"));
}

Path
EnginePaths::getEngineAssetDirectory()
{
  initialize();
  return Path("/Engine");
}

Path
EnginePaths::getEngineShaderDirectory()
{
  return getEngineAssetDirectory().join(Path("Shaders"));
}

Path
EnginePaths::getEditorContentDirectory()
{
  initialize();
  return Path("/Editor");
}

Path
EnginePaths::getSavedDirectory()
{
  initialize();
  return Path("/Saved");
}

Path
EnginePaths::getLogDirectory()
{
  return getSavedDirectory().join(Path("Logs"));
}

Path
EnginePaths::getConfigDirectory()
{
  return getSavedDirectory().join(Path("Config"));
}

String
EnginePaths::getEngineAssetExtension()
{
  return String(".chAss");
}

} // namespace chEngineSDK
