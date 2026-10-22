/************************************************************************/
/**
 * @file chProjectPackager.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Writes a folder that plays a project without the editor or the engine sources.
 */
/************************************************************************/
#include "chProjectPackager.h"

#if USING(CH_EDITOR)
#include "chAssetFile.h"
#include "chConfigFile.h"
#include "chDynamicLibrary.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chProjectDescriptor.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(PackagerLog, All);

namespace {
// The engine content folder of these holds sources the build already compiled.
const ANSICHAR* const kEngineSourceFolders[] = {"Shaders"};

enum class CopyKind : uint8
{
  Binary,
  Engine,
  Config,
  Asset
};

struct FileCopy
{
  Path source;
  Path destination;
  CopyKind kind;
};

Path
gameExecutablePath(const Path& directory)
{
  String fileName(ProjectPackager::GAME_EXECUTABLE_NAME);
  fileName += ProjectPackager::EXECUTABLE_EXTENSION;
  return directory.join(Path(std::move(fileName)));
}

bool
isSharedLibrary(const String& fileName)
{
  const StringView extension = DynamicLibrary::EXTENSION;
  if (fileName.size() > extension.size() &&
      StringView(fileName).substr(fileName.size() - extension.size()) == extension) {
    return true;
  }
#if !USING(CH_PLATFORM_WIN32)
  // Versioned names such as libSDL3.so.0, which the shorter names link to.
  String versioned(extension);
  versioned += '.';
  return fileName.find(versioned) != String::npos;
#else
  return false;
#endif
}

bool
hasProjectFile(const Path& directory)
{
  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(directory, files, directories);
  for (const Path& file : files) {
    if (file.getExtension() == EnginePaths::PROJECT_EXTENSION) {
      return true;
    }
  }
  return false;
}

/*
 * A previous package is replaced as a whole, so assets removed from the game do not stay
 * behind; anything else is never deleted.
 */
bool
prepareOutputDirectory(const Path& directory, String& outError)
{
  if (FileSystem::exists(directory)) {
    Vector<Path> files;
    Vector<Path> directories;
    FileSystem::getChildren(directory, files, directories);
    if (!files.empty() || !directories.empty()) {
      if (!hasProjectFile(directory)) {
        outError = "The output folder " + directory.toString() +
                   " is not empty and is not a package; choose another folder.";
        return false;
      }
      FileSystem::removeAll(directory);
    }
  }

  if (!FileSystem::createDirectories(directory)) {
    outError = "Could not create the output folder " + directory.toString() + ".";
    return false;
  }
  return true;
}

/*
 * skippedFolders are names of direct children of source that are left out.
 */
void
addFolder(const Path& source,
          const Path& destination,
          Span<const ANSICHAR* const> skippedFolders,
          CopyKind kind,
          Vector<FileCopy>& outCopies)
{
  FileSystem::forEachFileChildRecursive(source, [&](const Path& child) {
    if (!FileSystem::isFile(child)) {
      return;
    }
    const Path relative = FileSystem::relativePath(source, child);
    const String& relativeText = relative.toString();
    for (const ANSICHAR* folder : skippedFolders) {
      const StringView folderName(folder);
      if (relativeText.size() > folderName.size() &&
          StringView(relativeText).substr(0, folderName.size()) == folderName &&
          relativeText[folderName.size()] == '/') {
        return;
      }
    }
    outCopies.push_back({child, destination.join(relative), kind});
  });
}

bool
addBinaries(const PackageDesc& desc, Vector<FileCopy>& outCopies, String& outError)
{
  String executableName(desc.gameName);
  executableName += ProjectPackager::EXECUTABLE_EXTENSION;
  outCopies.push_back({gameExecutablePath(desc.binaryDirectory),
                       desc.outputDirectory.join(Path(std::move(executableName))),
                       CopyKind::Binary});

  // Only the folder itself: codecs live in a subfolder and are editor only.
  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(desc.binaryDirectory, files, directories);
  for (const Path& file : files) {
    String fileName = file.getFileName();
    if (isSharedLibrary(fileName)) {
      outCopies.push_back(
          {file, desc.outputDirectory.join(Path(std::move(fileName))), CopyKind::Binary});
    }
  }

  const Path shaders = desc.binaryDirectory.join(Path("Shaders"));
  if (!FileSystem::isDirectory(shaders)) {
    outError = "The build has no compiled shaders in " + shaders.toString() + ".";
    return false;
  }
  addFolder(shaders, desc.outputDirectory.join(Path("Shaders")), {}, CopyKind::Binary,
            outCopies);
  return true;
}

void
addConfig(const PackageDesc& desc, Vector<FileCopy>& outCopies)
{
  const Path sourceConfig = desc.projectDirectory.join(Path("Config"));
  if (!FileSystem::isDirectory(sourceConfig)) {
    return;
  }
  const Path packageConfig = desc.outputDirectory.join(Path("Config"));
  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(sourceConfig, files, directories);
  for (const Path& file : files) {
    String fileName = file.getFileName();
    // The editor's config and the package settings are never read by a game.
    if (file.getExtension() != ".ini" || fileName == "Editor.ini" ||
        fileName == ProjectPackager::PACKAGE_SETTINGS_FILE) {
      continue;
    }
    outCopies.push_back(
        {file, packageConfig.join(Path(std::move(fileName))), CopyKind::Config});
  }
}

bool
addAssets(const PackageDesc& desc, Vector<FileCopy>& outCopies, String& outError)
{
  const Path sourceAssets = desc.projectDirectory.join(Path("Assets"));
  const Path packageAssets = desc.outputDirectory.join(Path("Assets"));
  for (const Path& file : desc.assetFiles) {
    if (!FileSystem::isSubPath(sourceAssets, file)) {
      outError = "The asset " + file.toString() + " is not inside " +
                 sourceAssets.toString() + ".";
      return false;
    }
    outCopies.push_back(
        {file, packageAssets.join(FileSystem::relativePath(sourceAssets, file)),
         CopyKind::Asset});
  }
  return true;
}

bool
writeGameConfig(const PackageDesc& desc, String& outError)
{
  const Path packageConfig = desc.outputDirectory.join(Path("Config"));
  const Path gameConfigPath = packageConfig.join(Path("Game.ini"));
  ConfigFile gameConfig;
  gameConfig.load(gameConfigPath);
  gameConfig.setValue("Game", "StartupScene", desc.startupScene);
  // The window would otherwise carry the engine's title; a title the project chose wins.
  ConfigFile engineConfig;
  engineConfig.load(packageConfig.join(Path("Engine.ini")));
  if (!gameConfig.getValue("Window", "Title") && !engineConfig.getValue("Window", "Title")) {
    gameConfig.setValue("Window", "Title", desc.gameName);
  }
  if (!gameConfig.save(gameConfigPath)) {
    outError = "Could not write " + gameConfigPath.toString() + ".";
    return false;
  }

  // The project's own file, under the game's name, so its game modules come along.
  ProjectDescriptor descriptor;
  const Path sourceProjectFile = ProjectDescriptor::findProjectFile(desc.projectDirectory);
  if (!sourceProjectFile.empty() && !descriptor.load(sourceProjectFile)) {
    outError = "Could not read " + sourceProjectFile.toString() + ".";
    return false;
  }
  descriptor.name = desc.gameName;
  descriptor.engineVersion = CH_ENGINE_VERSION_STRING;
  String projectFileName(desc.gameName);
  projectFileName += EnginePaths::PROJECT_EXTENSION;
  const Path projectFile = desc.outputDirectory.join(Path(std::move(projectFileName)));
  if (!descriptor.save(projectFile)) {
    outError = "Could not write " + projectFile.toString() + ".";
    return false;
  }
  return true;
}
} // namespace

/*
 */
PackageResult
ProjectPackager::package(const PackageDesc& desc, PackageProgress* progress)
{
  PackageResult result;
  if (!isValidGameName(desc.gameName)) {
    result.error = "'" + desc.gameName + "' cannot be used as a file name.";
    return result;
  }
  if (!isGameBinaryDirectory(desc.binaryDirectory)) {
    result.error = "There is no " + gameExecutablePath(desc.binaryDirectory).toString() +
                   "; build the game first.";
    return result;
  }
  if (desc.outputDirectory.empty() ||
      FileSystem::isSubPath(desc.projectDirectory.join(Path("Assets")),
                            desc.outputDirectory) ||
      FileSystem::isSubPath(desc.binaryDirectory, desc.outputDirectory) ||
      FileSystem::isSubPath(desc.engineContentDirectory, desc.outputDirectory)) {
    result.error = "The output folder " + desc.outputDirectory.toString() +
                   " cannot be empty or inside the project assets, the build or the engine "
                   "content.";
    return result;
  }

  // Listed first, so the progress knows the total and nothing is copied when a source is
  // wrong.
  Vector<FileCopy> copies;
  if (!addBinaries(desc, copies, result.error) || !addAssets(desc, copies, result.error)) {
    return result;
  }
  addFolder(desc.engineContentDirectory,
            desc.outputDirectory.join(Path(String(EnginePaths::PACKAGE_ENGINE_FOLDER))),
            kEngineSourceFolders, CopyKind::Engine, copies);
  addConfig(desc, copies);

  if (progress) {
    progress->totalFiles.store(static_cast<uint32>(copies.size()), std::memory_order_relaxed);
  }
  if (!prepareOutputDirectory(desc.outputDirectory, result.error)) {
    return result;
  }

  for (const FileCopy& copy : copies) {
    if (progress && progress->bCancelRequested.load(std::memory_order_relaxed)) {
      // The folder was empty or a previous package, so nothing of the user's is lost.
      FileSystem::removeAll(desc.outputDirectory);
      result.bCancelled = true;
      result.error = "Cancelled.";
      return result;
    }
    // The editor data of an asset holds paths of this machine and means nothing to a game.
    const bool bStripEditorData =
        copy.kind == CopyKind::Asset &&
        copy.source.getExtension() == EnginePaths::getEngineAssetExtension();
    const bool bCopied =
        bStripEditorData
            ? AssetFile::rewriteWithEditorData(copy.source, copy.destination, StringView())
            : FileSystem::copyFile(copy.source, copy.destination);
    if (!bCopied) {
      result.error = "Could not copy " + copy.source.toString() + " to " +
                     copy.destination.toString() + ".";
      return result;
    }
    if (copy.kind == CopyKind::Binary) {
      ++result.binaryFileCount;
    }
    else if (copy.kind == CopyKind::Asset) {
      ++result.assetFileCount;
    }
    if (progress) {
      progress->copiedFiles.fetch_add(1, std::memory_order_relaxed);
    }
  }

  if (!writeGameConfig(desc, result.error)) {
    return result;
  }

  CH_LOG_INFO(PackagerLog, "Packaged {0} into {1}: {2} binary files, {3} assets.",
              desc.gameName, desc.outputDirectory, result.binaryFileCount,
              result.assetFileCount);
  result.bSucceeded = true;
  return result;
}

/*
 */
bool
ProjectPackager::isGameBinaryDirectory(const Path& directory)
{
  return !directory.empty() && FileSystem::isFile(gameExecutablePath(directory));
}

/*
 */
bool
ProjectPackager::isDebugBuild(const Path& directory)
{
  // CMake gives debug libraries a "d" postfix; engine modules have no "lib" prefix.
  String fileName("chCored");
  fileName += DynamicLibrary::EXTENSION;
  return FileSystem::isFile(directory.join(Path(std::move(fileName))));
}

/*
 */
bool
ProjectPackager::isValidGameName(StringView name)
{
  return ProjectDescriptor::isValidName(name);
}

/*
 */
void
ProjectPackager::collectAssets(Span<const UUID> roots,
                               const Function<const Vector<UUID>*(const UUID&)>& getReferences,
                               Vector<UUID>& outAssets,
                               Vector<UUID>& outMissing)
{
  outAssets.clear();
  outMissing.clear();

  // Runs once per package over a few hundred assets at most, so a set is clear enough.
  UnorderedSet<UUID> seen;
  Vector<UUID> pending;
  // Pushed backwards, so the first root comes out first.
  for (SIZE_T i = roots.size(); i > 0; --i) {
    if (seen.insert(roots[i - 1]).second) {
      pending.push_back(roots[i - 1]);
    }
  }
  while (!pending.empty()) {
    const UUID current = pending.back();
    pending.pop_back();

    const Vector<UUID>* references = getReferences(current);
    if (!references) {
      outMissing.push_back(current);
      continue;
    }
    outAssets.push_back(current);
    for (const UUID& reference : *references) {
      if (seen.insert(reference).second) {
        pending.push_back(reference);
      }
    }
  }
}

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
