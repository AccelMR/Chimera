/************************************************************************/
/**
 * @file chImportDialog.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Asks for a file and the options to import it with.
 */
/************************************************************************/
#include "chImportDialog.h"

#include "chAlgorithm.h"
#include "chAssetCodec.h"
#include "chEnginePaths.h"
#include "chLogger.h"
#include "chAssetManager.h"
#include "chModelAsset.h"
#include "chSkeletonAsset.h"
#include "chSTDThreading.h"
#include "chStringUtils.h"
#include "chTextureAsset.h"
#include "chUIHelpers.h"

#include "imgui.h"

#if USING(CH_DISPLAY_SDL3)
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#endif // USING(CH_DISPLAY_SDL3)

namespace chEngineSDK {
using namespace chUIHelpers;

CH_LOG_DECLARE_STATIC(ImportDialogLog, All);

namespace {
constexpr const ANSICHAR* kPopupName = "Import###ImportDialog";

constexpr const ANSICHAR* kSkeletonAuto = "Auto (reuse a compatible one)";
constexpr const ANSICHAR* kSkeletonCreateNew = "Create new";
// Indexed by ModelExpandMode.
constexpr const ANSICHAR* kExpandModeLabels[] = {"Whole model", "One object per node",
                                                 "One object per mesh"};

struct DialogState
{
  // Raw, because codecs live in plugins and must not be kept alive past their unloading;
  // the dialog only uses it while it is open. Not null from open() until the options close,
  // so a second import waits for the first.
  IAssetCodec* codec = nullptr;
  // The file dialog keeps pointers to these until it answers.
  String filterPattern;
  String defaultLocation;
#if USING(CH_DISPLAY_SDL3)
  SDL_DialogFileFilter filter{};
#endif // USING(CH_DISPLAY_SDL3)
  bool bBrowsing = false;
  // Written by the file dialog's callback, which may run on another thread; chosenFile is
  // only read after bFileChosen is seen true.
  String chosenFile;
  Atomic<bool> bFileChosen{false};
  Path sourceFile;
  Path assetFolder;
  String fileName;
  ImportSettings settings;
  bool bModelOptions = false;
  bool bTextureOptions = false;
  bool bOpenRequested = false;
  // The running import. The worker only touches task, progress and bReadSucceeded, and
  // bReadSucceeded is only read after bReadDone is seen true.
  UniquePtr<IImportTask> task;
  UniquePtr<ImportProgress> progress;
  Thread worker;
  bool bReadSucceeded = false;
  Atomic<bool> bReadDone{false};
  Event<void(const SPtr<IAsset>&)> onImported;
};

DialogState&
getState()
{
  static DialogState state;
  return state;
}

/*
 * Only models with bones read it. The project's skeletons are listed only while the combo
 * is open.
 */
void
renderSkeletonOption(ImportSettings& settings)
{
  SPtr<IAsset> existing;
  if (settings.skeletonMode == ImportSkeletonMode::Existing) {
    existing = AssetManager::instance().getAsset(settings.skeleton);
  }
  const ANSICHAR* preview = settings.skeletonMode == ImportSkeletonMode::CreateNew
                                ? kSkeletonCreateNew
                            : existing ? existing->getName()
                                       : kSkeletonAuto;
  if (ImGui::BeginCombo("Skeleton", preview)) {
    if (ImGui::Selectable(kSkeletonAuto, settings.skeletonMode == ImportSkeletonMode::Auto)) {
      settings.skeletonMode = ImportSkeletonMode::Auto;
    }
    if (ImGui::Selectable(kSkeletonCreateNew,
                          settings.skeletonMode == ImportSkeletonMode::CreateNew)) {
      settings.skeletonMode = ImportSkeletonMode::CreateNew;
    }
    for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
      if (!asset->isTypeOf<SkeletonAsset>()) {
        continue;
      }
      ImGui::PushID(asset.get());
      if (ImGui::Selectable(asset->getName(), asset == existing)) {
        settings.skeletonMode = ImportSkeletonMode::Existing;
        settings.skeleton = asset->getUUID();
      }
      ImGui::SetItemTooltip("%s", asset->getAssetPath());
      ImGui::PopID();
    }
    ImGui::EndCombo();
  }
  ImGui::SetItemTooltip("For models with bones. Auto reuses the skeleton an earlier import "
                        "of this file made, or any skeleton every bone of the file fits, so "
                        "they share animations; else a new one is made. A chosen skeleton "
                        "that does not fit also makes a new one.");
}

/*
 */
void
renderModelOptions(ImportSettings& settings)
{
  ImGui::SeparatorText("Model");
  int32 expandMode = static_cast<int32>(settings.expandMode);
  if (ImGui::Combo("Place as", &expandMode, kExpandModeLabels,
                   static_cast<int32>(std::size(kExpandModeLabels)))) {
    settings.expandMode = static_cast<ModelExpandMode>(expandMode);
  }
  ImGui::SetItemTooltip("What dropping the model in a scene makes. Hold Alt while dropping "
                        "to place the whole model instead.");

  ImGui::Checkbox("Split into separate parts", &settings.splitParts);
  ImGui::SetItemTooltip("Cuts each mesh into the pieces that do not touch, so props the file "
                        "merged into one mesh can be moved one by one. Pieces whose bounds "
                        "overlap stay together. More parts mean more draw calls.");

  ImGui::Checkbox("Mask materials by texture alpha", &settings.autoAlphaMask);
  ImGui::SetItemTooltip("Materials the file gives no alpha mode become masked when their "
                        "base color texture has transparent pixels.");

  renderSkeletonOption(settings);
}

/*
 */
void
renderTextureOptions(ImportSettings& settings)
{
  ImGui::SeparatorText("Texture");
  ImGui::Checkbox("sRGB (color data)", &settings.srgb);
  ImGui::SetItemTooltip("On for colors (base color, emissive); off for data such as normal "
                        "maps and masks.");
  ImGui::Checkbox("Generate mips", &settings.generateMips);
}

/*
 */
void
startImport(DialogState& state)
{
  state.task = state.codec->createImportTask(state.sourceFile, state.fileName, state.settings);
  if (!state.task) {
    CH_LOG_ERROR(ImportDialogLog, "Failed to import asset: {0}", state.sourceFile);
    state.codec = nullptr;
    return;
  }
  state.progress = chMakeUnique<ImportProgress>();
  state.bReadSucceeded = false;
  state.bReadDone.store(false, std::memory_order_relaxed);
  state.worker = Thread([&state]() {
    state.bReadSucceeded = state.task->readSource(*state.progress);
    state.bReadDone.store(true, std::memory_order_release);
  });
}

/*
 */
void
endImport(DialogState& state)
{
  state.task.reset();
  state.progress.reset();
  state.codec = nullptr;
}

/*
 */
void
renderProgress(DialogState& state)
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImVec2 corner(viewport->WorkPos.x + viewport->WorkSize.x,
                      viewport->WorkPos.y + viewport->WorkSize.y);
  const float margin = ImGui::GetFontSize();
  ImGui::SetNextWindowPos(ImVec2(corner.x - margin, corner.y - margin), ImGuiCond_Appearing,
                          ImVec2(1.0f, 1.0f));
  constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_AlwaysAutoResize |
                                      ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoDocking |
                                      ImGuiWindowFlags_NoSavedSettings;
  if (ImGui::Begin("Importing###ImportProgress", nullptr, kFlags)) {
    ImportProgress& progress = *state.progress;
    ImGui::Text("%s", state.sourceFile.getFileName().c_str());
    ImGui::TextDisabled("%s", progress.getLabel());
    const ImVec2 barSize(ImGui::GetFontSize() * 18.0f, 0.0f);
    const float fraction = progress.getFraction();
    if (fraction < 0.0f) {
      // A negative fraction draws ImGui's moving bar, for imports that cannot tell.
      ImGui::ProgressBar(-static_cast<float>(ImGui::GetTime()), barSize, "");
    }
    else {
      ImGui::ProgressBar(fraction, barSize);
    }

    const bool bCancelled = progress.isCancelled();
    ImGui::BeginDisabled(bCancelled);
    if (ImGui::Button(bCancelled ? "Cancelling..." : "Cancel")) {
      progress.cancel();
    }
    ImGui::EndDisabled();
  }
  ImGui::End();
}

/*
 */
void
showOptions(DialogState& state, const Path& file)
{
  const Vector<UUID> assetTypes = state.codec->getSupportedAssetTypes();
  state.sourceFile = file;
  state.fileName = file.getFileName(false);
  state.settings = ImportSettings{};
  state.bModelOptions =
      Algorithm::contains(assetTypes, AssetTypeTraits<ModelAsset>::getTypeId());
  state.bTextureOptions =
      Algorithm::contains(assetTypes, AssetTypeTraits<TextureAsset>::getTypeId());
  state.bOpenRequested = true;
}

#if USING(CH_DISPLAY_SDL3)
/*
 */
void SDLCALL
onFileChosen(void* userData, const ANSICHAR* const* fileList, int32 filter)
{
  CH_PARAMETER_UNUSED(userData);
  CH_PARAMETER_UNUSED(filter);
  DialogState& state = getState();
  state.chosenFile.clear();
  if (!fileList) {
    // SDL keeps the error per thread, so it must be read here.
    CH_LOG_ERROR(ImportDialogLog, "Could not open the file dialog: {0}", SDL_GetError());
  }
  else if (fileList[0]) {
    state.chosenFile = fileList[0];
  }
  state.bFileChosen.store(true, std::memory_order_release);
}
#endif // USING(CH_DISPLAY_SDL3)

/*
 */
void
takeChosenFile(DialogState& state)
{
  if (!state.bBrowsing || !state.bFileChosen.load(std::memory_order_acquire)) {
    return;
  }
  state.bFileChosen.store(false, std::memory_order_relaxed);
  state.bBrowsing = false;
  if (state.chosenFile.empty()) {
    state.codec = nullptr;
    return;
  }
  CH_LOG_INFO(ImportDialogLog, "Selected file: {0}", state.chosenFile);
  showOptions(state, Path(state.chosenFile));
}
} // namespace

/*
 */
void
ImportDialog::open(IAssetCodec& codec, const Path& assetFolder)
{
  DialogState& state = getState();
  if (state.codec) {
    CH_LOG_WARNING(ImportDialogLog, "An import is already open; finish it first.");
    return;
  }

#if USING(CH_DISPLAY_SDL3)
  state.codec = &codec;
  state.assetFolder = assetFolder;
  state.filterPattern = StringUtils::join(codec.getSupportedExtensions(), ";");
  // A trailing separator tells SDL the location is a folder, not a file name, and the
  // Windows shell does not parse forward slashes.
  state.defaultLocation = EnginePaths::getAbsoluteGameAssetDirectory().toString() + "/";
#if USING(CH_PLATFORM_WIN32)
  state.defaultLocation = StringUtils::replaceAllChars(state.defaultLocation, '/', '\\');
#endif // USING(CH_PLATFORM_WIN32)
  state.bBrowsing = true;

  state.filter = {"Supported files", state.filterPattern.c_str()};
  // The SDL3 platform backend keeps the SDL window id in PlatformHandle.
  SDL_Window* window = SDL_GetWindowFromID(static_cast<SDL_WindowID>(
      reinterpret_cast<SIZE_T>(ImGui::GetMainViewport()->PlatformHandle)));
  SDL_ShowOpenFileDialog(onFileChosen, nullptr, window, &state.filter, 1,
                         state.defaultLocation.c_str(), false);
#else
  CH_PARAMETER_UNUSED(codec);
  CH_PARAMETER_UNUSED(assetFolder);
  CH_LOG_ERROR(ImportDialogLog, "There is no file dialog without SDL3.");
#endif // USING(CH_DISPLAY_SDL3)
}

/*
 */
void
ImportDialog::render()
{
  DialogState& state = getState();
  takeChosenFile(state);
  if (state.task) {
    renderProgress(state);
  }
  if (state.bOpenRequested) {
    ImGui::OpenPopup(kPopupName);
    state.bOpenRequested = false;
  }

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (!ImGui::BeginPopupModal(kPopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  ImGui::Text("File: %s", state.sourceFile.getFileName().c_str());
  ImGui::Text("Into: %s", state.assetFolder.toString().c_str());
  if (state.bModelOptions) {
    renderModelOptions(state.settings);
  }
  if (state.bTextureOptions) {
    renderTextureOptions(state.settings);
  }

  ImGui::Separator();
  const float buttonWidth = ImGui::GetFontSize() * 7.0f;
  const bool bImport = ImGui::Button("Import", ImVec2(buttonWidth, 0.0f));
  ImGui::SameLine();
  const bool bCancel = ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)) ||
                       ImGui::IsKeyPressed(ImGuiKey_Escape);
  if (bImport || bCancel) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();

  if (bImport && state.codec) {
    startImport(state);
  }
  else if (bCancel) {
    state.codec = nullptr;
  }
}

/*
 */
void
ImportDialog::update()
{
  DialogState& state = getState();
  if (!state.task || !state.bReadDone.load(std::memory_order_acquire)) {
    return;
  }
  state.worker.join();

  if (state.progress->isCancelled()) {
    CH_LOG_INFO(ImportDialogLog, "Import of {0} cancelled", state.sourceFile);
  }
  else if (!state.bReadSucceeded) {
    CH_LOG_ERROR(ImportDialogLog, "Failed to import asset: {0}", state.sourceFile);
  }
  else if (const SPtr<IAsset> asset = state.task->createAssets(state.assetFolder)) {
    CH_LOG_INFO(ImportDialogLog, "Imported {0} as {1}", state.sourceFile, asset->getName());
    endImport(state);
    state.onImported(asset);
    return;
  }
  else {
    CH_LOG_ERROR(ImportDialogLog, "Failed to import asset: {0}", state.sourceFile);
  }
  endImport(state);
}

/*
 */
void
ImportDialog::shutDown()
{
  DialogState& state = getState();
  if (state.worker.joinable()) {
    state.progress->cancel();
    state.worker.join();
  }
  endImport(state);
}

/*
 */
HEvent
ImportDialog::onAssetImported(const Function<void(const SPtr<IAsset>&)>& callback)
{
  return getState().onImported.connect(callback);
}

} // namespace chEngineSDK
