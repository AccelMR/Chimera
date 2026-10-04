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

#include "chCommandLine.h"
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
  Path codecDirectory;
  Path engineRoot;
  Path projectRoot;
  Path absoluteGameAssetDirectory;
  Path gameAssetDirectory;
  Path engineAssetDirectory;
  Path shaderBinaryDirectory;
  Path editorContentDirectory;
  Path savedDirectory;
  Path logDirectory;
  Path configDirectory;
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
  const String requestedProject = CommandLine::getValue("project");

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

  state.codecDirectory = state.executableDirectory.join(Path("Codecs"));
  state.absoluteGameAssetDirectory = state.projectRoot.join(Path("Assets"));
  state.gameAssetDirectory = Path("/Game");
  state.engineAssetDirectory = Path("/Engine");
  state.shaderBinaryDirectory = state.executableDirectory.join(Path("Shaders"));
  state.editorContentDirectory = Path("/Editor");
  state.savedDirectory = Path("/Saved");
  state.logDirectory = state.savedDirectory.join(Path("Logs"));
  state.configDirectory = state.savedDirectory.join(Path("Config"));

  const Path savedDirectory = state.projectRoot.join(Path("Saved"));
  FileSystem::createDirectories(state.absoluteGameAssetDirectory);
  FileSystem::createDirectories(savedDirectory.join(Path("Logs")));
  FileSystem::createDirectories(savedDirectory.join(Path("Config")));

  FileSystem::mount("Engine", state.engineRoot.join(Path(kEngineContentFolder)));
#if USING(CH_EDITOR)
  FileSystem::mount("Editor", state.engineRoot.join(Path(kEditorContentFolder)));
#endif
  FileSystem::mount("Game", state.absoluteGameAssetDirectory, 0, true);
  FileSystem::mount("Saved", savedDirectory, 0, true);

  state.initialized = true;

  CH_LOG_INFO(EnginePathsLog, "Engine root: {0}", state.engineRoot);
  CH_LOG_INFO(EnginePathsLog, "Project: {0}", state.projectRoot);
}

const Path&
EnginePaths::getEngineRootDirectory()
{
  initialize();
  return pathState().engineRoot;
}

const Path&
EnginePaths::getProjectDirectory()
{
  initialize();
  return pathState().projectRoot;
}

const Path&
EnginePaths::getPluginDirectory()
{
  initialize();
  return pathState().executableDirectory;
}

const Path&
EnginePaths::getCodecDirectory()
{
  initialize();
  return pathState().codecDirectory;
}

const Path&
EnginePaths::getGameAssetDirectory()
{
  initialize();
  return pathState().gameAssetDirectory;
}

const Path&
EnginePaths::getAbsoluteGameAssetDirectory()
{
  initialize();
  return pathState().absoluteGameAssetDirectory;
}

const Path&
EnginePaths::getEngineAssetDirectory()
{
  initialize();
  return pathState().engineAssetDirectory;
}

const Path&
EnginePaths::getShaderBinaryDirectory()
{
  initialize();
  return pathState().shaderBinaryDirectory;
}

const Path&
EnginePaths::getEditorContentDirectory()
{
  initialize();
  return pathState().editorContentDirectory;
}

const Path&
EnginePaths::getSavedDirectory()
{
  initialize();
  return pathState().savedDirectory;
}

const Path&
EnginePaths::getLogDirectory()
{
  initialize();
  return pathState().logDirectory;
}

const Path&
EnginePaths::getConfigDirectory()
{
  initialize();
  return pathState().configDirectory;
}

} // namespace chEngineSDK
