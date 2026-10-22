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

#if USING(CH_EDITOR)
#include "chPlatformSystem.h"
#endif // USING(CH_EDITOR)

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(EnginePathsLog, All);

namespace {
const ANSICHAR* const kEngineContentFolder = "chCore/Content";
#if USING(CH_EDITOR)
const ANSICHAR* const kEditorContentFolder = "chEditor/Content";
constexpr StringView kUserDataOrganization = "Chimera";
constexpr StringView kEditorUserDataApplication = "Editor";
#endif // USING(CH_EDITOR)

struct PathState
{
  bool initialized = false;
  bool bPackaged = false;
  bool bHasProject = false;
  ProjectDescriptor projectDescriptor;
  Path projectFile;
  Path executableDirectory;
  Path codecDirectory;
  Path engineRoot;
  Path engineContentDirectory;
  Path projectRoot;
  Path absoluteGameAssetDirectory;
  Path gameAssetDirectory;
  Path engineAssetDirectory;
  Path shaderBinaryDirectory;
  Path editorContentDirectory;
  Path savedDirectory;
  Path logDirectory;
  Path configDirectory;
  Path projectConfigDirectory;
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

// The folder holding the project file name (a folder or the .chproject itself) gives; empty
// when name is empty or there is no project there.
Path
findProjectFileFor(const String& name)
{
  if (name.empty()) {
    return Path();
  }
  const Path project = FileSystem::absolutePath(Path(name));
  if (FileSystem::isFile(project) && project.getExtension() == EnginePaths::PROJECT_EXTENSION) {
    return project;
  }
  return ProjectDescriptor::findProjectFile(project);
}

// The project file of the project to open: -project, a package's own, then defaultProject.
// Empty when none of them names one.
Path
findProjectFile(const Path& executableDirectory, StringView defaultProject)
{
  const String requestedProject = CommandLine::getValue("project");
  if (!requestedProject.empty()) {
    const Path projectFile = findProjectFileFor(requestedProject);
    if (projectFile.empty()) {
      CH_EXCEPT(InvalidArgumentException,
                "-project must be a folder with a " +
                    String(EnginePaths::PROJECT_EXTENSION) +
                    " file, or the file itself: " + requestedProject);
    }
    return projectFile;
  }

  // A packaged game carries its project next to the executable, so it wins over the
  // folder a development build was given.
  const Path packagedProject = ProjectDescriptor::findProjectFile(executableDirectory);
  if (!packagedProject.empty()) {
    return packagedProject;
  }
  if (!defaultProject.empty()) {
    const Path projectFile = findProjectFileFor(String(defaultProject));
    if (!projectFile.empty()) {
      return projectFile;
    }
    CH_LOG_WARNING(EnginePathsLog, "The default project {0} has no {1} file; ignored.",
                   String(defaultProject), EnginePaths::PROJECT_EXTENSION);
  }
  return Path();
}
} // namespace

void
EnginePaths::initialize(StringView defaultProject, bool bProjectOptional)
{
#if !USING(CH_EDITOR)
  CH_PARAMETER_UNUSED(bProjectOptional);
#endif // !USING(CH_EDITOR)
  PathState& state = pathState();
  if (state.initialized) {
    return;
  }

  state.executableDirectory = FileSystem::getExecutableDirectory();
  const Path packageEngineDirectory =
      state.executableDirectory.join(Path(String(PACKAGE_ENGINE_FOLDER)));
  state.bPackaged = !ProjectDescriptor::findProjectFile(state.executableDirectory).empty() &&
                    FileSystem::isDirectory(packageEngineDirectory);
  if (state.bPackaged) {
    state.engineRoot = state.executableDirectory;
    state.engineContentDirectory = packageEngineDirectory;
  }
  else {
    state.engineRoot = findEngineRoot(state.executableDirectory);
    if (state.engineRoot.empty()) {
      CH_EXCEPT(InternalErrorException,
                "Could not find " + String(kEngineContentFolder) + " in " +
                    state.executableDirectory.toString() + " or any folder above it.");
    }
    state.engineContentDirectory = state.engineRoot.join(Path(kEngineContentFolder));
  }

  state.projectFile = findProjectFile(state.executableDirectory, defaultProject);
  if (!state.projectFile.empty()) {
    state.bHasProject = true;
    state.projectRoot = state.projectFile.getDirectory();
    if (!state.projectDescriptor.load(state.projectFile)) {
      CH_EXCEPT(InternalErrorException, "Could not read " + state.projectFile.toString());
    }
  }
#if USING(CH_EDITOR)
  else if (!bProjectOptional)
#else
  // Only the editor may start without a project.
  else
#endif // USING(CH_EDITOR)
  {
    // A game started without any project plays the folder it runs from.
    state.bHasProject = true;
    state.projectRoot = state.executableDirectory;
    state.projectDescriptor.name = state.executableDirectory.getFileName();
  }

  Path savedDirectory;
  if (state.bHasProject) {
    savedDirectory = state.projectRoot.join(Path("Saved"));
  }
#if USING(CH_EDITOR)
  else {
    // Without a project, what the editor writes goes to the user's folder instead.
    savedDirectory = getEditorUserDirectory();
  }
#endif // USING(CH_EDITOR)
  if (savedDirectory.empty()) {
    CH_EXCEPT(InternalErrorException, "There is no project and no user folder to write to.");
  }
  FileSystem::setBaseDirectory(state.bHasProject ? state.projectRoot : savedDirectory);

  state.codecDirectory = state.executableDirectory.join(Path("Codecs"));
  state.gameAssetDirectory = Path("/Game");
  state.engineAssetDirectory = Path("/Engine");
  state.shaderBinaryDirectory = state.executableDirectory.join(Path("Shaders"));
  state.editorContentDirectory = Path("/Editor");
  state.savedDirectory = Path("/Saved");
  state.logDirectory = state.savedDirectory.join(Path("Logs"));
  state.configDirectory = state.savedDirectory.join(Path("Config"));
  if (state.bHasProject) {
    state.absoluteGameAssetDirectory = state.projectRoot.join(Path("Assets"));
    state.projectConfigDirectory = state.projectRoot.join(Path("Config"));
    FileSystem::createDirectories(state.absoluteGameAssetDirectory);
  }
  FileSystem::createDirectories(savedDirectory.join(Path("Logs")));
  FileSystem::createDirectories(savedDirectory.join(Path("Config")));

  FileSystem::mount("Engine", state.engineContentDirectory);
#if USING(CH_EDITOR)
  if (!state.bPackaged) {
    FileSystem::mount("Editor", state.engineRoot.join(Path(kEditorContentFolder)));
  }
#endif
  if (state.bHasProject) {
    FileSystem::mount("Game", state.absoluteGameAssetDirectory, 0, true);
  }
  FileSystem::mount("Saved", savedDirectory, 0, true);

  state.initialized = true;

  CH_LOG_INFO(EnginePathsLog, "Engine root: {0}", state.engineRoot);
  if (state.bPackaged) {
    CH_LOG_INFO(EnginePathsLog, "Running from a package.");
  }
  if (!state.bHasProject) {
    CH_LOG_INFO(EnginePathsLog, "No project; writing to {0}", savedDirectory);
    return;
  }
  CH_LOG_INFO(EnginePathsLog, "Project: {0} ({1})", state.projectDescriptor.name,
              state.projectRoot);
  const String& version = state.projectDescriptor.engineVersion;
  if (!version.empty() && version != CH_ENGINE_VERSION_STRING) {
    CH_LOG_WARNING(EnginePathsLog, "The project was saved with engine {0}; this is {1}.",
                   version, CH_ENGINE_VERSION_STRING);
  }
}

bool
EnginePaths::hasProject()
{
  initialize();
  return pathState().bHasProject;
}

const Path&
EnginePaths::getProjectFile()
{
  initialize();
  return pathState().projectFile;
}

const ProjectDescriptor&
EnginePaths::getProjectDescriptor()
{
  initialize();
  return pathState().projectDescriptor;
}

#if USING(CH_EDITOR)
const Path&
EnginePaths::getEditorUserDirectory()
{
  // Asked only when needed, because the platform creates the folder.
  static const Path directory =
      PlatformSystem::getUserDataDirectory(kUserDataOrganization, kEditorUserDataApplication);
  return directory;
}
#endif // USING(CH_EDITOR)

const Path&
EnginePaths::getEngineRootDirectory()
{
  initialize();
  return pathState().engineRoot;
}

const Path&
EnginePaths::getEngineContentDirectory()
{
  initialize();
  return pathState().engineContentDirectory;
}

bool
EnginePaths::isPackaged()
{
  initialize();
  return pathState().bPackaged;
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

const Path&
EnginePaths::getProjectConfigDirectory()
{
  initialize();
  return pathState().projectConfigDirectory;
}

} // namespace chEngineSDK
