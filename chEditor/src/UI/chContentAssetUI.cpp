/************************************************************************/
/**
 * @file chContentAssetUI.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chContentAssetUI.h"

#include <cstring>
#include <ctime>

#if USING(CH_CODECS)
#include "chAssetCodec.h"
#include "chAssetCodecManager.h"
#endif // USING(CH_CODECS)

#include "chAlgorithm.h"
#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chLogger.h"
#include "chImGuiRenderer.h"
#include "chImportDialog.h"
#include "chITexture.h"
#include "chEditorScene.h"
#include "chEditorSelection.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chMath.h"
#include "chUIHelpers.h"

#include "chPrefabAsset.h"
#include "chMaterialAsset.h"
#include "chSceneAsset.h"
#include "chTextureAsset.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

#if USING(CH_DISPLAY_SDL3)

#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>

#endif // USING(CH_DISPLAY_SDL3)

namespace chEngineSDK {
using namespace chEngineSDK::chUIHelpers;

CH_LOG_DECLARE_STATIC(ContentAssetUILog, All);

namespace ContentAssetUIVars {
// Variable to control the visibility of the file explorer
static bool bShowContentWindow = false;
} // namespace ContentAssetUIVars
using namespace ContentAssetUIVars;

namespace {

// Tiles take the frame colors of the theme, so they stand out from the panel in any theme.
void
pushTileColors()
{
  ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                        ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));
}

NODISCARD ImVec4
getAssetStateColor(AssetState state) noexcept
{
  switch (state) {
  case AssetState::Loaded:
    return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
  case AssetState::Loading:
    return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
  case AssetState::Unloaded:
    return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
  case AssetState::Unloading:
    return ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
  case AssetState::Failed:
    return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
  default:
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
  }
}

NODISCARD const ANSICHAR*
getAssetStateString(AssetState state) noexcept
{
  switch (state) {
  case AssetState::Loaded:
    return "Loaded";
  case AssetState::Loading:
    return "Loading";
  case AssetState::Unloaded:
    return "Unloaded";
  case AssetState::Unloading:
    return "Unloading";
  case AssetState::Failed:
    return "Failed";
  default:
    return "Unknown";
  }
}

// Long names are cut so they fit under a grid icon.
using ShortName = ANSICHAR[13];

NODISCARD const ANSICHAR*
shortenName(const ANSICHAR* name, ShortName& outBuffer)
{
  if (std::strlen(name) <= 12) {
    return name;
  }
  std::memcpy(outBuffer, name, 9);
  std::memcpy(outBuffer + 9, "...", 4);
  return outBuffer;
}

} // namespace

/*
 */
ContentAssetUI::ContentAssetUI()
{
  refreshAssets();
  m_onAssetImported =
      ImportDialog::onAssetImported([this](const SPtr<IAsset>&) { refreshAssets(); });
  m_onSceneCreated =
      EditorScene::onAssetCreated([this](const SPtr<IAsset>&) { refreshAssets(); });
}

/*
 */
void
ContentAssetUI::refreshAssets()
{
  m_assets = AssetManager::instance().getAllAssets();
  rebuildFolderTree();
  m_needsFilterUpdate = true;
}

/*
 */
void
ContentAssetUI::renderContentAssetUI()
{
  renderDeleteConfirmationPopup();

  if (!ImGui::Begin(ICON_FA_FOLDER_OPEN "  Content Browser###ContentBrowser",
                    &bShowContentWindow)) {
    ImGui::End();
    return;
  }

  renderSearchBar();
  renderAssetTypeFilters();
  renderViewModeControls();

  // The folder tree on the left, the current folder's contents on the right.
  renderFolderTree();
  ImGui::SameLine();
  ImGui::BeginGroup();
  renderBreadcrumb();
  renderAssetDisplayArea();
  ImGui::EndGroup();

  if (m_requestedFolder != INVALID_FOLDER) {
    m_currentFolder = m_requestedFolder;
    m_requestedFolder = INVALID_FOLDER;
    m_needsFilterUpdate = true;
  }

  ImGui::End();
}

/*
 */
void
ContentAssetUI::renderSearchBar()
{
  ImGui::SetNextItemWidth(-1.0f);

  if (ImGui::InputTextWithHint("##search", "Search assets...", searchBuffer,
                               sizeof(searchBuffer))) {
    m_needsFilterUpdate = true;
  }

  ImGui::Separator();
}

/*
 */
void
ContentAssetUI::renderAssetTypeFilters()
{
  if (ImGui::Button(ICON_FA_TH_LARGE "  All")) {
    showAllTypes = true;
    showModels = showTextures = showMaterials = showOther = true;
    m_needsFilterUpdate = true;
  }
  ImGui::SameLine();

  if (ImGui::Button(ICON_FA_CUBE "  Models")) {
    showAllTypes = false;
    showModels = true;
    showTextures = showMaterials = showOther = false;
    m_needsFilterUpdate = true;
  }
  ImGui::SameLine();

  if (ImGui::Button(ICON_FA_IMAGE "  Textures")) {
    showAllTypes = false;
    showTextures = true;
    showModels = showMaterials = showOther = false;
    m_needsFilterUpdate = true;
  }
  ImGui::SameLine();

  if (ImGui::Button(ICON_FA_PALETTE "  Materials")) {
    showAllTypes = false;
    showMaterials = true;
    showModels = showTextures = showOther = false;
    m_needsFilterUpdate = true;
  }
  ImGui::SameLine();

  if (ImGui::Button(ICON_FA_FILE "  Other")) {
    showAllTypes = false;
    showOther = true;
    showModels = showTextures = showMaterials = false;
    m_needsFilterUpdate = true;
  }

  ImGui::Separator();
}

/*
 */
void
ContentAssetUI::renderViewModeControls()
{
  if (ImGui::RadioButton("Grid View", gridView)) {
    gridView = true;
  }
  ImGui::SameLine();

  if (ImGui::RadioButton("List View", !gridView)) {
    gridView = false;
  }

  if (gridView) {
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::SliderFloat("Size", &gridSize, 50.0f, 300.0f, "%.0f");
  }

  ImGui::Separator();
}

/*
 */
void
ContentAssetUI::renderAssetDisplayArea()
{
  if (m_needsFilterUpdate) {
    rebuildVisibleAssets();
  }

  ImGui::BeginChild("AssetArea", ImVec2(0, 0), false);

  if (gridView) {
    renderGridView();
  }
  else {
    renderListView();
  }

  handleEmptyAreaContextMenu();

  renderEmptyAreaContextMenu();

  ImGui::EndChild();
  // A folder inside the area is a smaller target, so ImGui gives it the drop instead.
  acceptPrefabDrop(getCurrentFolderPath());
}

/*
 */
void
ContentAssetUI::rebuildVisibleAssets()
{
  m_visibleAssets.clear();
  m_visibleAssets.reserve(m_assets.size());
  for (uint32 i = 0; i < static_cast<uint32>(m_assets.size()); ++i) {
    if (shouldShowAsset(m_assets[i])) {
      m_visibleAssets.push_back(i);
    }
  }
  m_needsFilterUpdate = false;
}

/*
 */
bool
ContentAssetUI::shouldShowAsset(const SPtr<IAsset>& asset) const
{
  return passesFolderFilter(asset) && passesSearchFilter(asset) && passesTypeFilter(asset);
}

/*
 */
bool
ContentAssetUI::passesFolderFilter(const SPtr<IAsset>& asset) const
{
  // A search looks in the subfolders too, as in Unreal.
  const StringView assetFolder = asset->getAssetPath();
  const String& folder = getCurrentFolderPath().toString();
  if (assetFolder == folder) {
    return true;
  }
  return searchBuffer[0] != '\0' && assetFolder.size() > folder.size() &&
         assetFolder.starts_with(folder) && assetFolder[folder.size()] == '/';
}

/*
 */
bool
ContentAssetUI::passesSearchFilter(const SPtr<IAsset>& asset) const
{
  return StringUtils::containsIgnoreCase(asset->getName(), searchBuffer);
}

/*
 */
bool
ContentAssetUI::passesTypeFilter(const SPtr<IAsset>& asset) const
{
  if (showAllTypes) {
    return true;
  }

  switch (UIHelpers::getIconFromAssetType(asset).type) {
  case AssetType::Model:
    return showModels;
  case AssetType::Texture:
    return showTextures;
  case AssetType::Material:
    return showMaterials;
  default:
    return showOther;
  }
}

/*
 */
void
ContentAssetUI::renderGridView()
{
  int32 columns = static_cast<int32>(ImGui::GetContentRegionAvail().x / (gridSize + 10.0f));
  if (columns < 1) {
    columns = 1;
  }

  // Subfolders come first. The clipper works on rows of the grid, which all have the same
  // height.
  const Vector<uint32>& folders = getVisibleSubfolders();
  const int32 folderCount = static_cast<int32>(folders.size());
  const int32 itemCount = folderCount + static_cast<int32>(m_visibleAssets.size());
  const int32 rowCount = (itemCount + columns - 1) / columns;

  ImGuiListClipper clipper;
  clipper.Begin(rowCount);
  while (clipper.Step()) {
    for (int32 row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
      const int32 first = row * columns;
      const int32 last = Math::min(first + columns, itemCount);
      for (int32 i = first; i < last; ++i) {
        if (i < folderCount) {
          renderGridFolderItem(folders[i], i - first);
        }
        else {
          renderGridAssetItem(m_assets[m_visibleAssets[i - folderCount]], i - first);
        }
      }
    }
  }
}

/*
 */
void
ContentAssetUI::renderListView()
{
  if (!ImGui::BeginTable("AssetTable", 4,
                         ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
                             ImGuiTableFlags_Borders)) {
    return;
  }

  setupTableColumns();

  const Vector<uint32>& folders = getVisibleSubfolders();
  const int32 folderCount = static_cast<int32>(folders.size());
  ImGuiListClipper clipper;
  clipper.Begin(folderCount + static_cast<int32>(m_visibleAssets.size()));
  while (clipper.Step()) {
    for (int32 i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
      if (i < folderCount) {
        renderListFolderItem(folders[i]);
      }
      else {
        renderListAssetItem(m_assets[m_visibleAssets[i - folderCount]]);
      }
    }
  }

  ImGui::EndTable();
}

/*
 */
void
ContentAssetUI::setupTableColumns()
{
  ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 40.0f);
  ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
  ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
  ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 80.0f);
  ImGui::TableHeadersRow();
}

/*
 */
void
ContentAssetUI::renderGridAssetItem(const SPtr<IAsset>& asset, int32 column)
{
  if (column > 0) {
    ImGui::SameLine();
  }

  // The asset pointer is unique and stable, so the widgets inside can use fixed labels.
  ImGui::PushID(asset.get());
  ImGui::BeginGroup();

  renderAssetIconButton(asset);
  renderAssetNameInGrid(asset);

  ImGui::EndGroup();

  renderAssetStateIndicator(asset);
  handleAssetContextMenu(asset);
  renderAssetTooltip(asset);

  ImGui::PopID();
}

/*
 */
void
ContentAssetUI::renderListAssetItem(const SPtr<IAsset>& asset)
{
  ImGui::TableNextRow();
  ImGui::PushID(asset.get());

  ImGui::TableSetColumnIndex(0);
  ImGui::TextUnformatted(UIHelpers::getIconFromAssetType(asset).icon);

  ImGui::TableSetColumnIndex(1);
  if (!renderInlineRename(asset)) {
    renderSelectableAssetName(asset);
  }

  handleAssetContextMenu(asset);

  ImGui::TableSetColumnIndex(2);
  ImGui::TextUnformatted(asset->getTypeName());

  ImGui::TableSetColumnIndex(3);
  const AssetState state = asset->getState();
  ImGui::TextColored(getAssetStateColor(state), "%s", getAssetStateString(state));

  ImGui::PopID();
}

/*
 */
void
ContentAssetUI::renderAssetIconButton(const SPtr<IAsset>& asset)
{
  const AssetIcon assetIcon = UIHelpers::getIconFromAssetType(asset);
  const ImVec2 buttonSize(gridSize, gridSize);

  pushTileColors();

  const uint64 thumbnail = assetIcon.type == AssetType::Texture ? getThumbnail(asset) : 0;

  if (thumbnail != 0) {
    ImGui::ImageButton("##asset", static_cast<ImTextureID>(thumbnail), buttonSize);
  }
  else {
    ImGui::Button("##asset", buttonSize);
  }
  AssetDragDrop::source(*asset);
  if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
    EditorSelection::setSelectedAsset(asset);
  }
  if (EditorSelection::getSelectedAsset() == asset) {
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                        ImGui::GetColorU32(ImGuiCol_HeaderActive),
                                        ImGui::GetStyle().FrameRounding, 0, 2.0f);
  }
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    openAsset(asset);
  }

  ImGui::PopStyleColor(3);

  const ImVec2 buttonMin = ImGui::GetItemRectMin();
  const ImVec2 iconPos =
      ImVec2(buttonMin.x + (gridSize - 32) * 0.5f, buttonMin.y + (gridSize - 32) * 0.5f - 10);

  ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 32.0f, iconPos,
                                      IM_COL32(255, 255, 255, 255), assetIcon.icon);
}

/*
 */
void
ContentAssetUI::renderAssetNameInGrid(const SPtr<IAsset>& asset)
{
  if (renderInlineRename(asset)) {
    return;
  }

  ShortName shortName;
  const ANSICHAR* displayName = shortenName(asset->getName(), shortName);

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                       (gridSize - ImGui::CalcTextSize(displayName).x) * 0.5f);

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                        ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));

  ImGui::Button(displayName);

  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    startInlineRename(asset);
  }

  ImGui::PopStyleColor(3);
}

/*
 */
void
ContentAssetUI::renderSelectableAssetName(const SPtr<IAsset>& asset)
{
  if (ImGui::Selectable(asset->getName(), EditorSelection::getSelectedAsset() == asset,
                        ImGuiSelectableFlags_SpanAllColumns)) {
    EditorSelection::setSelectedAsset(asset);
  }
  AssetDragDrop::source(*asset);

  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    // Scenes and prefabs open like files in a file browser; other assets have nothing to
    // open.
    if (asset->isTypeOf<SceneAsset>() || asset->isTypeOf<PrefabAsset>()) {
      openAsset(asset);
    }
    else {
      startInlineRename(asset);
    }
  }
}

/*
 */
void
ContentAssetUI::renderAssetStateIndicator(const SPtr<IAsset>& asset)
{
  const ImVec4 stateColor = getAssetStateColor(asset->getState());
  const ImVec2 groupMin = ImGui::GetItemRectMin();
  const ImVec2 groupMax = ImGui::GetItemRectMax();

  // Scaled with the grid size, within limits so it is never too small or too big.
  const float offset = Math::max(6.0f, Math::min(gridSize * 0.08f, 15.0f));
  const float radius = Math::max(2.0f, Math::min(gridSize * 0.05f, 8.0f));

  ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(groupMax.x - offset, groupMin.y + offset),
                                              radius,
                                              ImGui::ColorConvertFloat4ToU32(stateColor));
}

/*
 */
void
ContentAssetUI::handleAssetContextMenu(const SPtr<IAsset>& asset)
{
  if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    ImGui::OpenPopup("AssetContext");
  }

  if (ImGui::BeginPopup("AssetContext")) {
    renderAssetContextMenu(asset);
    ImGui::EndPopup();
  }
}

/*
 */
void
ContentAssetUI::renderAssetTooltip(const SPtr<IAsset>& asset)
{
  if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
    return;
  }

  const AssetEditorData& editorData = asset->getEditorData();
  const std::time_t createdAtTimeT = static_cast<std::time_t>(editorData.creationTime);

  // std::localtime returns a pointer to shared static data, so it is not thread-safe; the
  // platform versions fill a local struct instead.
  ANSICHAR createdAtStr[64] = {};
  struct tm timeInfo = {};
#if USING(CH_PLATFORM_WIN32)
  const bool hasLocalTime = localtime_s(&timeInfo, &createdAtTimeT) == 0;
#else
  const bool hasLocalTime = localtime_r(&createdAtTimeT, &timeInfo) != nullptr;
#endif
  if (editorData.creationTime != 0 && hasLocalTime) {
    std::strftime(createdAtStr, sizeof(createdAtStr), "%Y-%m-%d %H:%M:%S", &timeInfo);
  }
  else {
    StringUtils::copyToBuffer(createdAtStr, "Unknown");
  }

  ImGui::BeginTooltip();
  ImGui::Text("UUID: %s", asset->getUUID().toString().c_str());
  ImGui::Text("Name: %s", asset->getName());
  ImGui::Text("Type: %s", asset->getTypeName());
  ImGui::Text("Type UUID: %s", asset->getAssetTypeId().toString().c_str());
  ImGui::Text("Created At: %s", createdAtStr);
  ImGui::Text("State: %s", getAssetStateString(asset->getState()));
  ImGui::Text("Source: %s%s%s", editorData.sourceFile.toString().c_str(),
              editorData.sourcePart.empty() ? "" : " # ", editorData.sourcePart.c_str());
  ImGui::Text("Asset Path: %s", asset->getAssetPath());
  ImGui::EndTooltip();
}

/*
 */
uint64
ContentAssetUI::getThumbnail(const SPtr<IAsset>& asset)
{
  const UUID& uuid = asset->getUUID();
  auto it = m_assetThumbnails.find(uuid);
  if (it != m_assetThumbnails.end()) {
    return it->second.second;
  }

  // Stored even when it fails, so a broken texture is not loaded again every frame.
  Pair<SPtr<ITexture>, uint64>& thumbnail = m_assetThumbnails[uuid];

  SPtr<TextureAsset> textureAsset = std::static_pointer_cast<TextureAsset>(asset);

  // The texture is unloaded again afterwards only if it was loaded here.
  const bool loadedHere = textureAsset->isUnloaded();
  if (loadedHere && !AssetManager::instance().syncLoadAsset(textureAsset)) {
    CH_LOG_ERROR(ContentAssetUILog, "Failed to load texture asset: {0}", asset->getName());
    return 0;
  }

  SPtr<ITexture> texture = textureAsset->getTexture();
  const uint64 textureId =
      texture ? ImGuiRenderer::getTextureId(*texture) : 0;
  if (textureId != 0) {
    // Keeping the texture keeps the thumbnail valid after the asset is unloaded below.
    thumbnail = {std::move(texture), textureId};
  }
  else {
    CH_LOG_ERROR(ContentAssetUILog, "Failed to create the thumbnail of {0}.",
                 asset->getName());
  }

  if (loadedHere) {
    AssetManager::instance().unloadAsset(asset);
  }
  return thumbnail.second;
}

/*
 */
void
ContentAssetUI::renderDeleteConfirmationPopup()
{
  if (!m_showDeleteConfirmation || !m_assetToDelete) {
    m_showDeleteConfirmation = false;
    return;
  }
  ImGui::OpenPopup("Delete Asset?");

  if (ImGui::BeginPopupModal("Delete Asset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    if (m_assetToDelete) {
      ImGui::Text("Are you sure you want to delete this asset?");
      ImGui::Separator();
      ImGui::Text("Name: %s", m_assetToDelete->getName());
      ImGui::Text("Type: %s", m_assetToDelete->getTypeName());
      ImGui::Separator();
      ImGui::Text("This action cannot be undone!");

      if (ImGui::Button("Delete", ImVec2(120, 0))) {
        const Path assetFile = m_assetToDelete->getFilePath();
        const bool bRemovedCorrectly = FileSystem::removeFile(assetFile);
        const Path metaFile = AssetMeta::getPath(assetFile);
        if (bRemovedCorrectly && FileSystem::isFile(metaFile) &&
            !FileSystem::removeFile(metaFile)) {
          CH_LOG_WARNING(ContentAssetUILog, "Failed to delete {0}", metaFile);
        }

        if (bRemovedCorrectly) {
          m_assetThumbnails.erase(m_assetToDelete->getUUID());
          AssetManager::instance().removeAsset(m_assetToDelete->getUUID());
          refreshAssets();
          CH_LOG_DEBUG(ContentAssetUILog, "Deleted asset: {0}", m_assetToDelete->getName());
        }
        else {
          CH_LOG_ERROR(ContentAssetUILog, "Failed to delete asset file: {0}",
                       m_assetToDelete->getName());
        }

        m_assetToDelete = nullptr;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        m_assetToDelete = nullptr;
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::EndPopup();
  }
}

/*
 */
void
ContentAssetUI::loadAsset(const SPtr<IAsset>& asset)
{
  if (!AssetManager::instance().syncLoadAsset(asset)) {
    CH_LOG_ERROR(ContentAssetUILog, "Failed to load asset: {0}", asset->getName());
  }
}

/*
 */
void
ContentAssetUI::renderAssetContextMenu(const SPtr<IAsset>& asset)
{
  if (!asset) {
    return;
  }

  if (asset->isTypeOf<SceneAsset>()) {
    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "  Open")) {
      openAsset(asset);
    }
    ImGui::Separator();
  }

  if (asset->isTypeOf<PrefabAsset>()) {
    if (ImGui::MenuItem(ICON_FA_EDIT "  Edit Prefab")) {
      openAsset(asset);
    }
    ImGui::Separator();
  }

  if (ImGui::MenuItem(ICON_FA_DOWNLOAD "  Load")) {
    loadAsset(asset);
  }

  if (ImGui::MenuItem(ICON_FA_EJECT "  Unload", nullptr, false, asset->isLoaded())) {
    CH_LOG_DEBUG(ContentAssetUILog, "Unloading asset: {0}", asset->getName());
    AssetManager::instance().unloadAsset(asset->getUUID());
  }

  ImGui::Separator();

  if (ImGui::MenuItem(ICON_FA_I_CURSOR "  Rename")) {
    startInlineRename(asset);
    ImGui::CloseCurrentPopup();
  }

  ImGui::Separator();

  if (ImGui::MenuItem(ICON_FA_TRASH_ALT "  Delete", nullptr, false, asset->isUnloaded())) {
    m_assetToDelete = asset;
    m_showDeleteConfirmation = true;

    ImGui::CloseCurrentPopup();
  }
}

/*
 */
void
ContentAssetUI::startInlineRename(const SPtr<IAsset>& asset)
{
  if (!asset) {
    return;
  }

  m_isRenaming = true;
  m_renamingAsset = asset;
  m_renameFocusRequested = true;

  StringUtils::copyToBuffer(m_renameBuffer, asset->getName());

  CH_LOG_DEBUG(ContentAssetUILog, "Started inline rename for asset: {0}", asset->getName());
}

/*
 */
void
ContentAssetUI::finishInlineRename()
{
  if (!m_isRenaming || !m_renamingAsset) {
    return;
  }

  const String newName = StringUtils::trim(String(m_renameBuffer));

  if (!newName.empty() && newName != m_renamingAsset->getName()) {
    if (!AssetManager::instance().renameAsset(m_renamingAsset, newName.c_str())) {
      CH_LOG_ERROR(ContentAssetUILog, "Failed to rename asset to: {0}", newName);
    }
    // The new name may no longer match the search.
    m_needsFilterUpdate = true;
  }

  cancelInlineRename();
}

/*
 */
void
ContentAssetUI::cancelInlineRename()
{
  m_isRenaming = false;
  m_renamingAsset = nullptr;
  m_renameFocusRequested = false;
  std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
}

/*
 */
bool
ContentAssetUI::renderInlineRename(const SPtr<IAsset>& asset)
{
  if (!m_isRenaming || m_renamingAsset != asset) {
    return false;
  }

  const ImVec2 textSize = ImGui::CalcTextSize(m_renameBuffer);
  const float inputWidth = Math::max(textSize.x + 20.0f, 100.0f);

  // Styled to look close to the plain name it replaces.
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

  ImGui::SetNextItemWidth(inputWidth);

  if (m_renameFocusRequested) {
    ImGui::SetKeyboardFocusHere();
    m_renameFocusRequested = false;
  }

  const bool enterPressed =
      ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer),
                       ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

  if (enterPressed) {
    finishInlineRename();
  }
  else if (ImGui::IsItemDeactivated()) {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      cancelInlineRename();
    }
    else {
      finishInlineRename();
    }
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    cancelInlineRename();
  }

  ImGui::PopStyleVar(2);

  return true;
}

/*
 */
void
ContentAssetUI::handleEmptyAreaContextMenu()
{
  if (ImGui::IsAnyItemHovered() || !ImGui::IsWindowHovered()) {
    return;
  }

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    ImGui::OpenPopup("EmptyAreaContextMenu");
  }
}

/*
 */
void
ContentAssetUI::rebuildFolderTree()
{
  const Path previous = m_folders.empty() ? EnginePaths::getGameAssetDirectory()
                                          : getCurrentFolderPath();
  m_folders.clear();
  m_folders.push_back({.path = EnginePaths::getGameAssetDirectory(),
                       .name = "Game",
                       .parent = INVALID_FOLDER,
                       .children = {}});
  scanFolder(0);

  const uint32 found = findFolder(previous);
  m_currentFolder = found == INVALID_FOLDER ? 0 : found;
  m_requestedFolder = INVALID_FOLDER;
  m_needsFilterUpdate = true;
}

/*
 */
void
ContentAssetUI::scanFolder(uint32 folderIndex)
{
  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(m_folders[folderIndex].path, files, directories);
  Algorithm::sort(directories, [](const Path& a, const Path& b) {
    return a.toString() < b.toString();
  });

  for (const Path& directory : directories) {
    const uint32 childIndex = static_cast<uint32>(m_folders.size());
    m_folders.push_back({.path = directory,
                         .name = directory.getFileName(),
                         .parent = folderIndex,
                         .children = {}});
    m_folders[folderIndex].children.push_back(childIndex);
    scanFolder(childIndex);
  }
}

/*
 */
uint32
ContentAssetUI::findFolder(const Path& path) const
{
  for (uint32 i = 0; i < static_cast<uint32>(m_folders.size()); ++i) {
    if (m_folders[i].path.toString() == path.toString()) {
      return i;
    }
  }
  return INVALID_FOLDER;
}

/*
 */
const Vector<uint32>&
ContentAssetUI::getVisibleSubfolders() const
{
  // A search lists assets from every subfolder, so the folders themselves are hidden.
  static const Vector<uint32> kNoFolders;
  return searchBuffer[0] != '\0' ? kNoFolders : m_folders[m_currentFolder].children;
}

/*
 */
const Path&
ContentAssetUI::getCurrentFolderPath() const
{
  return m_folders[m_currentFolder].path;
}

/*
 */
void
ContentAssetUI::requestFolder(uint32 folderIndex)
{
  m_requestedFolder = folderIndex;
}

/*
 * One prefab per selected object, made after the frame's UI, because saving refreshes the
 * asset list.
 */
void
ContentAssetUI::acceptPrefabDrop(const Path& folder)
{
  if (!ImGui::BeginDragDropTarget()) {
    return;
  }
  if (const SPtr<GameObject> gameObject = AssetDragDrop::acceptGameObject()) {
    // A drag selects its row when it starts, so this only matters if something changed the
    // selection during the drag.
    if (!EditorSelection::isSelected(*gameObject)) {
      EditorSelection::setSelectedGameObject(gameObject);
    }
    EditorScene::requestCreatePrefabs(folder);
  }
  ImGui::EndDragDropTarget();
}

/*
 */
void
ContentAssetUI::renderFolderTree()
{
  // Resizable by its right edge; the width given is only the first one.
  ImGui::BeginChild("FolderTree", ImVec2(ImGui::GetFontSize() * 12.0f, 0.0f),
                    ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
  if (!m_folders.empty()) {
    renderFolderTreeNode(0);
  }
  m_bRevealCurrentFolder = false;
  ImGui::EndChild();
}

/*
 */
void
ContentAssetUI::renderFolderTreeNode(uint32 folderIndex)
{
  const FolderNode& folder = m_folders[folderIndex];

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                             ImGuiTreeNodeFlags_OpenOnDoubleClick |
                             ImGuiTreeNodeFlags_SpanAvailWidth;
  if (folder.children.empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }
  if (folderIndex == m_currentFolder) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }
  if (folderIndex == 0) {
    flags |= ImGuiTreeNodeFlags_DefaultOpen;
  }

  if (m_bRevealCurrentFolder) {
    for (uint32 i = m_folders[m_currentFolder].parent; i != INVALID_FOLDER;
         i = m_folders[i].parent) {
      if (i == folderIndex) {
        ImGui::SetNextItemOpen(true);
        break;
      }
    }
  }

  ImGui::PushID(static_cast<int32>(folderIndex));
  const bool bOpen = ImGui::TreeNodeEx("##folder", flags, "%s %s",
                                       UIHelpers::getFolderIcon(false), folder.name.c_str());
  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    requestFolder(folderIndex);
  }
  acceptPrefabDrop(folder.path);
  if (bOpen) {
    for (const uint32 child : folder.children) {
      renderFolderTreeNode(child);
    }
    ImGui::TreePop();
  }
  ImGui::PopID();
}

/*
 */
void
ContentAssetUI::renderBreadcrumb()
{
  // From the current folder up to /Game, then drawn from the top.
  constexpr uint32 kMaxDepth = 32;
  Array<uint32, kMaxDepth> chain{};
  uint32 depth = 0;
  for (uint32 i = m_currentFolder; i != INVALID_FOLDER && depth < kMaxDepth;
       i = m_folders[i].parent) {
    chain[depth++] = i;
  }

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  for (uint32 i = depth; i > 0; --i) {
    const uint32 folderIndex = chain[i - 1];
    ImGui::PushID(static_cast<int32>(folderIndex));
    if (ImGui::SmallButton(m_folders[folderIndex].name.c_str())) {
      requestFolder(folderIndex);
    }
    ImGui::PopID();
    if (i > 1) {
      ImGui::SameLine(0.0f, 2.0f);
      ImGui::TextDisabled(">");
      ImGui::SameLine(0.0f, 2.0f);
    }
  }
  ImGui::PopStyleColor();
  ImGui::Separator();
}

/*
 */
void
ContentAssetUI::renderGridFolderItem(uint32 folderIndex, int32 column)
{
  if (column > 0) {
    ImGui::SameLine();
  }

  const FolderNode& folder = m_folders[folderIndex];
  ImGui::PushID(static_cast<int32>(folderIndex));
  ImGui::BeginGroup();

  pushTileColors();
  ImGui::Button("##folder", ImVec2(gridSize, gridSize));
  ImGui::PopStyleColor(3);
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    requestFolder(folderIndex);
    m_bRevealCurrentFolder = true;
  }
  acceptPrefabDrop(folder.path);

  const ImVec2 buttonMin = ImGui::GetItemRectMin();
  const ImVec2 iconPos =
      ImVec2(buttonMin.x + (gridSize - 32) * 0.5f, buttonMin.y + (gridSize - 32) * 0.5f - 10);
  ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 32.0f, iconPos,
                                      IM_COL32(230, 190, 90, 255),
                                      UIHelpers::getFolderIcon(false));

  ShortName shortName;
  const ANSICHAR* displayName = shortenName(folder.name.c_str(), shortName);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                       (gridSize - ImGui::CalcTextSize(displayName).x) * 0.5f);
  ImGui::TextUnformatted(displayName);

  ImGui::EndGroup();
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", folder.path.toString().c_str());
  }
  ImGui::PopID();
}

/*
 */
void
ContentAssetUI::renderListFolderItem(uint32 folderIndex)
{
  const FolderNode& folder = m_folders[folderIndex];
  ImGui::TableNextRow();
  ImGui::PushID(static_cast<int32>(folderIndex));

  ImGui::TableSetColumnIndex(0);
  ImGui::TextUnformatted(UIHelpers::getFolderIcon(false));

  ImGui::TableSetColumnIndex(1);
  ImGui::Selectable(folder.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    requestFolder(folderIndex);
    m_bRevealCurrentFolder = true;
  }
  acceptPrefabDrop(folder.path);

  ImGui::TableSetColumnIndex(2);
  ImGui::TextUnformatted("Folder");

  ImGui::PopID();
}

/*
 */
void
ContentAssetUI::openAsset(const SPtr<IAsset>& asset)
{
  if (asset->isTypeOf<SceneAsset>()) {
    EditorScene::requestOpen(std::static_pointer_cast<SceneAsset>(asset));
  }
  else if (asset->isTypeOf<PrefabAsset>()) {
    EditorScene::requestEditPrefab(std::static_pointer_cast<PrefabAsset>(asset));
  }
}

/*
 */
void
ContentAssetUI::createFolder()
{
  const Path& parent = getCurrentFolderPath();
  String name = "NewFolder";
  for (uint32 suffix = 1; findFolder(parent.join(Path(name))) != INVALID_FOLDER; ++suffix) {
    name = StringUtils::format("NewFolder{0}", suffix);
  }

  const Path folder = parent.join(Path(name));
  if (!FileSystem::createDirectory(folder)) {
    CH_LOG_ERROR(ContentAssetUILog, "Failed to create the folder {0}", folder);
    return;
  }
  rebuildFolderTree();
  const uint32 created = findFolder(folder);
  if (created != INVALID_FOLDER) {
    m_currentFolder = created;
    m_bRevealCurrentFolder = true;
  }
}

/*
 */
void
ContentAssetUI::renderEmptyAreaContextMenu()
{
  if (!ImGui::BeginPopup("EmptyAreaContextMenu")) {
    return;
  }

  if (ImGui::MenuItem(ICON_FA_SYNC_ALT "  Refresh")) {
    refreshAssets();
    CH_LOG_INFO(ContentAssetUILog, "Refreshed asset list.");
  }

  ImGui::Separator();

  if (ImGui::BeginMenu(ICON_FA_FILE_IMPORT "  Import Asset")) {
    ImGui::Separator();
#if USING(CH_CODECS)
    AssetCodecManager& codecManager = AssetCodecManager::instance();
    AssetManager& assetManager = AssetManager::instance();
    for (const auto& codec : codecManager.getAllCodecs()) {
      for (const auto& assetType : codec->getSupportedAssetTypes()) {
        const String& codecTypeName = assetManager.getAssetTypeName(assetType);
        if (ImGui::MenuItem(codecTypeName.c_str())) {
          ImportDialog::open(*codec, getCurrentFolderPath());
          ImGui::EndMenu();
          ImGui::EndPopup();
          return;
        }
      }
    }
#endif // USING(CH_CODECS)
    ImGui::EndMenu();
  }

  ImGui::Separator();

  if (ImGui::BeginMenu(ICON_FA_PLUS "  Create")) {
    if (ImGui::MenuItem(ICON_FA_FOLDER_PLUS "  Folder")) {
      createFolder();
    }
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_PALETTE "  Material")) {
      // Saved at once, so a model or object can reference it before "Save All".
      AssetManager& assetManager = AssetManager::instance();
      const Path& folder = getCurrentFolderPath();
      const SPtr<MaterialAsset> material =
          assetManager.createAsset<MaterialAsset>(
              assetManager.makeUniqueAssetName(folder, "M_New"), folder).lock();
      if (!material || !assetManager.saveAsset(material)) {
        CH_LOG_ERROR(ContentAssetUILog, "Failed to create a material.");
      }
      refreshAssets();
    }
    ImGui::EndMenu();
  }

  ImGui::EndPopup();
}
} // namespace chEngineSDK
