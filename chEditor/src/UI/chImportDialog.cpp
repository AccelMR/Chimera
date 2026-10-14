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
#include "chModelAsset.h"
#include "chTextureAsset.h"
#include "chUIHelpers.h"

#include "imgui.h"

namespace chEngineSDK {
using namespace chUIHelpers;

CH_LOG_DECLARE_STATIC(ImportDialogLog, All);

namespace {
constexpr const ANSICHAR* kPopupName = "Import###ImportDialog";

// Indexed by ModelExpandMode.
constexpr const ANSICHAR* kExpandModeLabels[] = {"Whole model", "One object per node",
                                                 "One object per mesh"};

struct DialogState
{
  // Raw, because codecs live in plugins and must not be kept alive past their unloading;
  // the dialog only uses it while it is open.
  IAssetCodec* codec = nullptr;
  Path sourceFile;
  Path assetFolder;
  String fileName;
  ImportSettings settings;
  bool bModelOptions = false;
  bool bTextureOptions = false;
  bool bOpenRequested = false;
  Event<void(const SPtr<IAsset>&)> onImported;
};

DialogState&
getState()
{
  static DialogState state;
  return state;
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
runImport(DialogState& state)
{
  const SPtr<IAsset> asset = state.codec->importAsset(state.sourceFile, state.fileName,
                                                      state.assetFolder, state.settings);
  if (!asset) {
    CH_LOG_ERROR(ImportDialogLog, "Failed to import asset: {0}", state.sourceFile);
    return;
  }
  CH_LOG_INFO(ImportDialogLog, "Imported {0} as {1}", state.sourceFile, asset->getName());
  state.onImported(asset);
}
} // namespace

/*
 */
void
ImportDialog::open(IAssetCodec& codec, const Path& assetFolder)
{
  const Path file = UIHelpers::openFileExplorer(EnginePaths::getAbsoluteGameAssetDirectory(),
                                                codec.getSupportedExtensions());
  // openFileExplorer already logs a cancelled dialog or a dialog error.
  if (file.empty()) {
    return;
  }

  DialogState& state = getState();
  const Vector<UUID> assetTypes = codec.getSupportedAssetTypes();
  state.codec = &codec;
  state.sourceFile = file;
  state.assetFolder = assetFolder;
  state.fileName = file.getFileName(false);
  state.settings = ImportSettings{};
  state.bModelOptions =
      Algorithm::contains(assetTypes, AssetTypeTraits<ModelAsset>::getTypeId());
  state.bTextureOptions =
      Algorithm::contains(assetTypes, AssetTypeTraits<TextureAsset>::getTypeId());
  state.bOpenRequested = true;
}

/*
 */
void
ImportDialog::render()
{
  DialogState& state = getState();
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
    runImport(state);
  }
  if (bImport || bCancel) {
    state.codec = nullptr;
  }
}

/*
 */
HEvent
ImportDialog::onAssetImported(const Function<void(const SPtr<IAsset>&)>& callback)
{
  return getState().onImported.connect(callback);
}

} // namespace chEngineSDK
