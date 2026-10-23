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
#include "chAssetInspectorUI.h"
#include "chAnimationClipAsset.h"
#include "chAnimationPreviewUI.h"
#include "chAssetManager.h"
#include "chAssetThumbnails.h"
#include "chConfigFile.h"
#include "chLogger.h"
#include "chImportDialog.h"
#include "chEditorScene.h"
#include "chEditorSelection.h"
#include "chEngineConfig.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chMath.h"
#include "chProjectPackager.h"
#include "chUIHelpers.h"

#include "chPrefabAsset.h"
#include "chMaterialAsset.h"
#include "chSceneAsset.h"
#include "chScriptAsset.h"

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

constexpr const ANSICHAR* kFolderPayload = "CH_FOLDER";

/*
 * Names every file system takes: no separators or characters Windows refuses, and no
 * trailing dot or space, which Windows drops.
 */
bool
isValidFileName(StringView name)
{
  if (name.empty() || name == "." || name == ".." || name.back() == '.' ||
      name.back() == ' ') {
    return false;
  }
  for (const ANSICHAR c : name) {
    if (static_cast<uint8>(c) < 32 || StringView("/\\:*?\"<>|").find(c) != StringView::npos) {
      return false;
    }
  }
  return true;
}

NODISCARD bool
isInsideFolder(StringView path, StringView folder)
{
  return path == folder || (path.size() > folder.size() && path.starts_with(folder) &&
                            path[folder.size()] == '/');
}

String
getScenePath(const IAsset& scene)
{
  String path(scene.getAssetPath());
  path += '/';
  path += scene.getName();
  return path;
}

/*
 * The template names its class after the script, so the name must be a Lua identifier.
 */
bool
isLuaIdentifier(StringView name)
{
  if (name.empty() || (name[0] >= '0' && name[0] <= '9')) {
    return false;
  }
  for (const ANSICHAR c : name) {
    const bool bLetter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    if (!bLetter && !(c >= '0' && c <= '9') && c != '_') {
      return false;
    }
  }
  return true;
}

String
makeScriptTemplate(const String& name)
{
  return StringUtils::format(
      "-- {0}: runs on every GameObject that has it while the game plays.\n"
      "-- The line below tells code editors what self holds (gameObject, transform...).\n"
      "---@class {0} : Script\n"
      "local {0} = {{}}\n"
      "\n"
      "-- Runs once, before the first update.\n"
      "function {0}:onStart()\n"
      "end\n"
      "\n"
      "-- Runs every frame; deltaTime is in seconds.\n"
      "function {0}:update(deltaTime)\n"
      "end\n"
      "\n"
      "-- Runs at the fixed rate, for logic that must not depend on the frame rate.\n"
      "-- function {0}:fixedUpdate(deltaTime)\n"
      "-- end\n"
      "\n"
      "-- Runs when the object is destroyed or the game stops.\n"
      "-- function {0}:onDestroy()\n"
      "-- end\n"
      "\n"
      "return {0}\n",
      name);
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
  AssetManager& assetManager = AssetManager::instance();
  m_assets = assetManager.getAllAssets();
  Algorithm::removeAllIf(m_selectedAssets, [&assetManager](const SPtr<IAsset>& asset) {
    return !assetManager.getAsset(asset->getUUID());
  });
  rebuildFolderTree();
  m_needsFilterUpdate = true;
}

/*
 */
void
ContentAssetUI::renderContentAssetUI()
{
  renderDeleteConfirmationPopup();
  renderDeleteFolderPopup();
  renderMoveConfirmationPopup();
  syncAssetSelection();

  if (!ImGui::Begin(ICON_FA_FOLDER_OPEN "  Content Browser###ContentBrowser",
                    &bShowContentWindow)) {
    ImGui::End();
    return;
  }

  handleRenameShortcut();
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
    m_selectedFolder = INVALID_FOLDER;
    // Assets of a folder no longer shown would move with a drag without being seen.
    if (m_selectedAssets.size() > 1) {
      m_selectedAssets.assign(1, m_selectedAssets.back());
    }
    m_needsFilterUpdate = true;
  }
  if (m_bMoveRequested && !m_bOpenMoveConfirmation) {
    applyPendingMove();
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
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_COG "##ThumbnailSettings")) {
    ImGui::OpenPopup("ThumbnailSettings");
  }
  ImGui::SetItemTooltip("Thumbnail settings");
  if (ImGui::BeginPopup("ThumbnailSettings")) {
    AssetThumbnails::renderSettings();
    ImGui::EndPopup();
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
  acceptFolderDrop(getCurrentFolderPath());
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
  if (const int32 renameItem = findRenameFocusItem(); renameItem >= 0) {
    clipper.IncludeItemByIndex(renameItem / columns);
  }
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
  if (!ImGui::BeginTable("AssetTable", 3,
                         ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
                             ImGuiTableFlags_Borders)) {
    return;
  }

  setupTableColumns();

  const Vector<uint32>& folders = getVisibleSubfolders();
  const int32 folderCount = static_cast<int32>(folders.size());
  ImGuiListClipper clipper;
  clipper.Begin(folderCount + static_cast<int32>(m_visibleAssets.size()));
  if (const int32 renameItem = findRenameFocusItem(); renameItem >= 0) {
    clipper.IncludeItemByIndex(renameItem);
  }
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
  const uint64 thumbnail = AssetThumbnails::get(asset);
  if (thumbnail != 0) {
    const float side = ImGui::GetTextLineHeight();
    ImGui::Image(static_cast<ImTextureID>(thumbnail), ImVec2(side, side));
  }
  else {
    ImGui::TextUnformatted(UIHelpers::getIconFromAssetType(asset).icon);
  }

  ImGui::TableSetColumnIndex(1);
  if (!renderInlineRename(asset)) {
    renderSelectableAssetName(asset);
  }

  handleAssetContextMenu(asset);

  ImGui::TableSetColumnIndex(2);
  ImGui::TextUnformatted(asset->getTypeName());


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

  const uint64 thumbnail = AssetThumbnails::get(asset);

  if (thumbnail != 0) {
    ImGui::ImageButton("##asset", static_cast<ImTextureID>(thumbnail), buttonSize);
  }
  else {
    ImGui::Button("##asset", buttonSize);
  }
  const bool bSelected = isAssetSelected(asset);
  AssetDragDrop::source(*asset, bSelected ? static_cast<uint32>(m_selectedAssets.size()) : 1);
  handleAssetClick(asset);
  if (bSelected) {
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                        ImGui::GetColorU32(ImGuiCol_HeaderActive),
                                        ImGui::GetStyle().FrameRounding, 0, 2.0f);
  }
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    openAsset(asset);
  }

  ImGui::PopStyleColor(3);

  // Over a thumbnail the type icon goes small in a corner, so it does not hide the picture.
  const ImVec2 buttonMin = ImGui::GetItemRectMin();
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  if (thumbnail != 0) {
    const float iconSize = ImGui::GetFontSize();
    const ImVec2 padding = ImGui::GetStyle().FramePadding;
    const ImVec2 iconPos(buttonMin.x + padding.x * 2.0f, buttonMin.y + padding.y * 2.0f);
    drawList->AddText(ImGui::GetFont(), iconSize, ImVec2(iconPos.x + 1.0f, iconPos.y + 1.0f),
                      IM_COL32(0, 0, 0, 200), assetIcon.icon);
    drawList->AddText(ImGui::GetFont(), iconSize, iconPos, IM_COL32(255, 255, 255, 230),
                      assetIcon.icon);
    return;
  }
  const ImVec2 iconPos =
      ImVec2(buttonMin.x + (gridSize - 32) * 0.5f, buttonMin.y + (gridSize - 32) * 0.5f - 10);
  drawList->AddText(ImGui::GetFont(), 32.0f, iconPos, IM_COL32(255, 255, 255, 255),
                    assetIcon.icon);
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
  const bool bSelected = isAssetSelected(asset);
  ImGui::Selectable(asset->getName(), bSelected, ImGuiSelectableFlags_SpanAllColumns);
  AssetDragDrop::source(*asset, bSelected ? static_cast<uint32>(m_selectedAssets.size()) : 1);
  handleAssetClick(asset);

  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    openAsset(asset);
  }
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
  ImGui::Text("Source: %s%s%s", editorData.sourceFile.toString().c_str(),
              editorData.sourcePart.empty() ? "" : " # ", editorData.sourcePart.c_str());
  ImGui::Text("Asset Path: %s", asset->getAssetPath());
  ImGui::EndTooltip();
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

        if (bRemovedCorrectly) {
          AssetInspectorUI::close(*m_assetToDelete);
          AnimationPreviewUI::close(*m_assetToDelete);
          if (EditorSelection::getSelectedAsset() == m_assetToDelete) {
            EditorSelection::setSelectedAsset(nullptr);
          }
          AssetThumbnails::forget(*m_assetToDelete);
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
  else if (!asset->isTypeOf<SceneAsset>()) {
    if (ImGui::MenuItem(ICON_FA_FILE_ALT "  Inspect")) {
      openAsset(asset);
    }
    if (AnimationPreviewUI::canOpen(*asset) &&
        ImGui::MenuItem(ICON_FA_RUNNING "  Animation Preview")) {
      AnimationPreviewUI::open(asset);
    }
    ImGui::Separator();
  }

  if (AssetThumbnails::canHaveThumbnail(*asset) &&
      ImGui::MenuItem(ICON_FA_SYNC_ALT "  Regenerate Thumbnail")) {
    AssetThumbnails::regenerate(*asset);
  }

  if (ImGui::MenuItem(ICON_FA_I_CURSOR "  Rename", "F2")) {
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

  cancelInlineRename();
  m_isRenaming = true;
  m_renamingAsset = asset;
  m_renameFocusRequested = true;
  StringUtils::copyToBuffer(m_renameBuffer, asset->getName());
}

/*
 */
void
ContentAssetUI::startFolderRename(uint32 folderIndex)
{
  // /Game is the mount of the project's Assets folder, so it keeps its name.
  if (folderIndex == 0 || folderIndex >= m_folders.size()) {
    return;
  }

  cancelInlineRename();
  // A search hides the folders, and with them the tile that holds the field.
  if (searchBuffer[0] != '\0') {
    searchBuffer[0] = '\0';
    m_needsFilterUpdate = true;
  }
  m_isRenaming = true;
  m_renamingFolder = folderIndex;
  m_renameFocusRequested = true;
  StringUtils::copyToBuffer(m_renameBuffer, m_folders[folderIndex].name);
}

/*
 */
void
ContentAssetUI::finishInlineRename()
{
  if (!m_isRenaming) {
    return;
  }

  const String newName = StringUtils::trim(String(m_renameBuffer));
  if (m_renamingAsset && !newName.empty() && newName != m_renamingAsset->getName()) {
    if (!isValidFileName(newName)) {
      CH_LOG_ERROR(ContentAssetUILog, "{0} is not a valid asset name.", newName);
    }
    else if (AssetManager::instance().renameAsset(m_renamingAsset, newName.c_str())) {
      EditorScene::onAssetMoved(*m_renamingAsset);
      // Nothing was written in a script just made, so its class takes the new name.
      if (m_bRenameRewritesScript && m_renamingAsset->isTypeOf<ScriptAsset>() &&
          isLuaIdentifier(newName)) {
        const SPtr<ScriptAsset> script = std::static_pointer_cast<ScriptAsset>(m_renamingAsset);
        script->setSource(makeScriptTemplate(newName));
        if (!AssetManager::instance().saveAsset(script)) {
          CH_LOG_ERROR(ContentAssetUILog, "Failed to save the script {0}.", newName);
        }
      }
    }
    else {
      CH_LOG_ERROR(ContentAssetUILog, "Failed to rename asset to: {0}", newName);
    }
    // The new name may no longer match the search.
    m_needsFilterUpdate = true;
  }
  else if (m_renamingFolder != INVALID_FOLDER && !newName.empty() &&
           newName != m_folders[m_renamingFolder].name) {
    const FolderNode& folder = m_folders[m_renamingFolder];
    if (isValidFileName(newName)) {
      requestFolderMove(folder.path, m_folders[folder.parent].path.join(Path(newName)));
    }
    else {
      CH_LOG_ERROR(ContentAssetUILog, "{0} is not a valid folder name.", newName);
    }
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
  m_renamingFolder = INVALID_FOLDER;
  m_renameFocusRequested = false;
  m_bRenameRewritesScript = false;
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
  renderRenameField();
  return true;
}

/*
 */
bool
ContentAssetUI::renderFolderRename(uint32 folderIndex)
{
  if (!m_isRenaming || m_renamingFolder != folderIndex) {
    return false;
  }
  renderRenameField();
  return true;
}

/*
 */
void
ContentAssetUI::renderRenameField()
{
  const ImVec2 textSize = ImGui::CalcTextSize(m_renameBuffer);
  const float inputWidth = Math::max(textSize.x + 20.0f, 100.0f);

  // Styled to look close to the plain name it replaces.
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

  ImGui::SetNextItemWidth(inputWidth);

  if (m_renameFocusRequested) {
    ImGui::SetScrollHereY();
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
}

/*
 */
int32
ContentAssetUI::findRenameFocusItem() const
{
  if (!m_renameFocusRequested) {
    return -1;
  }
  const Vector<uint32>& folders = getVisibleSubfolders();
  for (SIZE_T i = 0; i < folders.size(); ++i) {
    if (folders[i] == m_renamingFolder) {
      return static_cast<int32>(i);
    }
  }
  if (!m_renamingAsset) {
    return -1;
  }
  for (SIZE_T i = 0; i < m_visibleAssets.size(); ++i) {
    if (m_assets[m_visibleAssets[i]] == m_renamingAsset) {
      return static_cast<int32>(folders.size() + i);
    }
  }
  return -1;
}

/*
 */
void
ContentAssetUI::handleRenameShortcut()
{
  if (m_isRenaming || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
      ImGui::GetIO().WantTextInput || !ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
    return;
  }
  if (m_selectedFolder != INVALID_FOLDER) {
    startFolderRename(m_selectedFolder);
    return;
  }
  // Only an asset the browser shows, since the field is drawn in its tile.
  const SPtr<IAsset>& selected = EditorSelection::getSelectedAsset();
  if (selected && passesFolderFilter(selected)) {
    startInlineRename(selected);
  }
}

/*
 */
void
ContentAssetUI::folderDragSource(uint32 folderIndex)
{
  if (folderIndex == 0 || !ImGui::BeginDragDropSource()) {
    return;
  }
  // ImGui copies the bytes; the path is read back on drop.
  const String& path = m_folders[folderIndex].path.toString();
  ImGui::SetDragDropPayload(kFolderPayload, path.data(), path.size());
  ImGui::Text("%s %s", UIHelpers::getFolderIcon(false), m_folders[folderIndex].name.c_str());
  ImGui::EndDragDropSource();
}

/*
 * Prefabs are made after the frame's UI, because saving refreshes the asset list.
 */
void
ContentAssetUI::acceptFolderDrop(const Path& folder)
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
  else if (const SPtr<IAsset> asset = AssetDragDrop::acceptAnyAsset()) {
    requestAssetMove(asset, folder);
  }
  else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kFolderPayload)) {
    const Path dragged(String(static_cast<const ANSICHAR*>(payload->Data),
                              static_cast<SIZE_T>(payload->DataSize)));
    requestFolderMove(dragged, folder.join(Path(dragged.getFileName())));
  }
  ImGui::EndDragDropTarget();
}

/*
 */
void
ContentAssetUI::requestAssetMove(const SPtr<IAsset>& asset, const Path& folder)
{
  if (!asset) {
    return;
  }
  m_moveAssets.clear();
  if (m_selectedAssets.size() > 1 && isAssetSelected(asset)) {
    for (const SPtr<IAsset>& selected : m_selectedAssets) {
      if (folder.toString() != selected->getAssetPath()) {
        m_moveAssets.push_back(selected);
      }
    }
  }
  else if (folder.toString() != asset->getAssetPath()) {
    m_moveAssets.push_back(asset);
  }
  if (m_moveAssets.empty()) {
    return;
  }
  m_moveFolder = Path();
  m_moveTarget = folder;
  m_bMoveRequested = true;
  findBrokenScenePaths();
  m_bOpenMoveConfirmation = !m_moveWarnings.empty();
}

/*
 */
void
ContentAssetUI::requestFolderMove(const Path& folder, const Path& newFolder)
{
  if (folder.toString() == newFolder.toString()) {
    return;
  }
  m_moveAssets.clear();
  m_moveFolder = folder;
  m_moveTarget = newFolder;
  m_bMoveRequested = true;
  findBrokenScenePaths();
  m_bOpenMoveConfirmation = !m_moveWarnings.empty();
}

/*
 */
void
ContentAssetUI::findBrokenScenePaths()
{
  m_moveWarnings.clear();

  Vector<String> movedScenes;
  if (m_moveFolder.empty()) {
    for (const SPtr<IAsset>& asset : m_moveAssets) {
      if (asset->isTypeOf<SceneAsset>()) {
        movedScenes.push_back(getScenePath(*asset));
      }
    }
  }
  else {
    for (const SPtr<IAsset>& asset : m_assets) {
      if (asset->isTypeOf<SceneAsset>() &&
          isInsideFolder(asset->getAssetPath(), m_moveFolder.toString())) {
        movedScenes.push_back(getScenePath(*asset));
      }
    }
  }
  if (movedScenes.empty()) {
    return;
  }

  // The editor's own startup scene follows the open scene (EditorScene::onAssetMoved).
  if (const String* startup =
          EngineConfig::getFile("Game").getValue("Game", "StartupScene")) {
    if (Algorithm::contains(movedScenes, *startup)) {
      m_moveWarnings.push_back(
          StringUtils::format("Game.ini [Game] StartupScene names {0}", *startup));
    }
  }

  ConfigFile packageSettings;
  const Path packagePath = EnginePaths::getProjectConfigDirectory().join(
      Path(String(ProjectPackager::PACKAGE_SETTINGS_FILE)));
  if (FileSystem::exists(packagePath) && packageSettings.load(packagePath)) {
    if (const String* scenes = packageSettings.getValue("Package", "Scenes")) {
      for (const String& scene : StringUtils::splitString(*scenes, ',')) {
        if (Algorithm::contains(movedScenes, scene)) {
          m_moveWarnings.push_back(
              StringUtils::format("Package.ini [Package] Scenes lists {0}", scene));
        }
      }
    }
  }
}

/*
 */
void
ContentAssetUI::renderMoveConfirmationPopup()
{
  if (m_bOpenMoveConfirmation) {
    ImGui::OpenPopup("Move Scene?");
    m_bOpenMoveConfirmation = false;
  }
  if (!ImGui::BeginPopupModal("Move Scene?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  ImGui::TextUnformatted("These settings name a scene by its path and will not find it:");
  for (const String& warning : m_moveWarnings) {
    ImGui::BulletText("%s", warning.c_str());
  }
  ImGui::TextUnformatted("Assets that use it reference it by UUID and keep working.");
  ImGui::Separator();

  if (ImGui::Button("Move Anyway", ImVec2(120, 0))) {
    m_bMoveConfirmed = true;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel", ImVec2(120, 0))) {
    m_bMoveRequested = false;
    m_moveAssets.clear();
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();
}

/*
 */
void
ContentAssetUI::applyPendingMove()
{
  // Waits for the question; Cancel clears the request.
  if (!m_moveWarnings.empty() && !m_bMoveConfirmed) {
    return;
  }
  m_bMoveRequested = false;
  m_bMoveConfirmed = false;
  m_moveWarnings.clear();

  AssetManager& assetManager = AssetManager::instance();
  if (m_moveFolder.empty()) {
    for (const SPtr<IAsset>& asset : m_moveAssets) {
      if (assetManager.moveAsset(asset, m_moveTarget)) {
        EditorScene::onAssetMoved(*asset);
      }
    }
    m_moveAssets.clear();
    refreshAssets();
    return;
  }

  // The current folder follows when it is the moved one or inside it.
  const String& oldText = m_moveFolder.toString();
  const String& currentText = getCurrentFolderPath().toString();
  String newCurrent;
  if (isInsideFolder(currentText, oldText)) {
    newCurrent = m_moveTarget.toString();
    newCurrent += StringView(currentText).substr(oldText.size());
  }

  if (!assetManager.moveFolder(m_moveFolder, m_moveTarget)) {
    return;
  }
  for (const SPtr<IAsset>& asset : m_assets) {
    if (isInsideFolder(asset->getAssetPath(), m_moveTarget.toString())) {
      EditorScene::onAssetMoved(*asset);
    }
  }
  refreshAssets();
  if (!newCurrent.empty()) {
    const uint32 found = findFolder(Path(newCurrent));
    if (found != INVALID_FOLDER) {
      m_currentFolder = found;
      m_bRevealCurrentFolder = true;
    }
  }
  m_selectedFolder = findFolder(m_moveTarget);
}

/*
 */
void
ContentAssetUI::syncAssetSelection()
{
  const SPtr<IAsset>& active = EditorSelection::getSelectedAsset();
  if (!active) {
    m_selectedAssets.clear();
  }
  else if (!Algorithm::contains(m_selectedAssets, active)) {
    m_selectedAssets.assign(1, active);
    m_selectionAnchor = active;
  }
}

/*
 */
bool
ContentAssetUI::isAssetSelected(const SPtr<IAsset>& asset) const
{
  return Algorithm::contains(m_selectedAssets, asset);
}

/*
 */
void
ContentAssetUI::selectOnlyAsset(const SPtr<IAsset>& asset)
{
  m_selectedAssets.assign(1, asset);
  m_selectionAnchor = asset;
  m_selectedFolder = INVALID_FOLDER;
  EditorSelection::setSelectedAsset(asset);
}

/*
 */
void
ContentAssetUI::handleAssetClick(const SPtr<IAsset>& asset)
{
  const ImGuiIO& io = ImGui::GetIO();
  const bool bPartOfSelection = m_selectedAssets.size() > 1 && isAssetSelected(asset);

  if (!ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
    if (bPartOfSelection && !io.KeyCtrl && !io.KeyShift && ImGui::IsItemHovered() &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] <
            io.MouseDragThreshold * io.MouseDragThreshold) {
      selectOnlyAsset(asset);
    }
    return;
  }

  m_selectedFolder = INVALID_FOLDER;
  if (io.KeyCtrl) {
    m_selectionAnchor = asset;
    if (isAssetSelected(asset)) {
      Algorithm::removeFirst(m_selectedAssets, asset);
      EditorSelection::setSelectedAsset(m_selectedAssets.empty() ? nullptr
                                                                 : m_selectedAssets.back());
    }
    else {
      m_selectedAssets.push_back(asset);
      EditorSelection::setSelectedAsset(asset);
    }
    return;
  }

  if (io.KeyShift && m_selectionAnchor) {
    int32 anchorIndex = -1;
    int32 clickedIndex = -1;
    for (SIZE_T i = 0; i < m_visibleAssets.size(); ++i) {
      const SPtr<IAsset>& visible = m_assets[m_visibleAssets[i]];
      if (visible == m_selectionAnchor) {
        anchorIndex = static_cast<int32>(i);
      }
      if (visible == asset) {
        clickedIndex = static_cast<int32>(i);
      }
    }
    if (anchorIndex >= 0 && clickedIndex >= 0) {
      m_selectedAssets.clear();
      const int32 first = Math::min(anchorIndex, clickedIndex);
      const int32 last = Math::max(anchorIndex, clickedIndex);
      for (int32 i = first; i <= last; ++i) {
        const SPtr<IAsset>& visible = m_assets[m_visibleAssets[i]];
        if (visible != asset) {
          m_selectedAssets.push_back(visible);
        }
      }
      // The clicked one goes last, as the active asset.
      m_selectedAssets.push_back(asset);
      EditorSelection::setSelectedAsset(asset);
      return;
    }
  }

  if (!bPartOfSelection) {
    selectOnlyAsset(asset);
  }
}

/*
 */
void
ContentAssetUI::renderDeleteFolderPopup()
{
  if (m_bOpenDeleteFolder) {
    ImGui::OpenPopup("Delete Folder?");
    m_bOpenDeleteFolder = false;
  }
  if (!ImGui::BeginPopupModal("Delete Folder?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  ImGui::Text("Delete %s and everything in it?", m_folderToDelete.toString().c_str());
  ImGui::Text("Assets inside: %u", m_folderToDeleteAssets);
  ImGui::TextUnformatted("Objects and assets that use them lose them. This cannot be undone!");
  ImGui::Separator();

  if (ImGui::Button("Delete", ImVec2(120, 0))) {
    Vector<SPtr<IAsset>> inside;
    for (const SPtr<IAsset>& asset : m_assets) {
      if (isInsideFolder(asset->getAssetPath(), m_folderToDelete.toString())) {
        inside.push_back(asset);
      }
    }
    const bool bCurrentInside =
        isInsideFolder(getCurrentFolderPath().toString(), m_folderToDelete.toString());
    const Path parent = m_folderToDelete.getDirectory();

    AssetManager& assetManager = AssetManager::instance();
    assetManager.removeFolder(m_folderToDelete);
    // Also after a partial failure: what the manager forgot is gone from disk.
    for (const SPtr<IAsset>& asset : inside) {
      if (assetManager.getAsset(asset->getUUID())) {
        continue;
      }
      AssetInspectorUI::close(*asset);
      AnimationPreviewUI::close(*asset);
      AssetThumbnails::forget(*asset);
      if (EditorSelection::getSelectedAsset() == asset) {
        EditorSelection::setSelectedAsset(nullptr);
      }
    }
    refreshAssets();
    if (bCurrentInside) {
      const uint32 found = findFolder(parent);
      m_currentFolder = found == INVALID_FOLDER ? 0 : found;
      m_needsFilterUpdate = true;
    }
    m_folderToDelete = Path();
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel", ImVec2(120, 0))) {
    m_folderToDelete = Path();
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();
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
  const Path previousSelected =
      m_selectedFolder != INVALID_FOLDER ? m_folders[m_selectedFolder].path : Path();
  // The index it holds may point at another folder after the scan.
  if (m_renamingFolder != INVALID_FOLDER) {
    cancelInlineRename();
  }
  m_folders.clear();
  m_folders.push_back({.path = EnginePaths::getGameAssetDirectory(),
                       .name = "Game",
                       .parent = INVALID_FOLDER,
                       .children = {}});
  scanFolder(0);

  const uint32 found = findFolder(previous);
  m_currentFolder = found == INVALID_FOLDER ? 0 : found;
  m_requestedFolder = INVALID_FOLDER;
  m_selectedFolder = previousSelected.empty() ? INVALID_FOLDER : findFolder(previousSelected);
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
  folderDragSource(folderIndex);
  acceptFolderDrop(folder.path);
  handleFolderContextMenu(folderIndex);
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
    acceptFolderDrop(m_folders[folderIndex].path);
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
  if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
    m_selectedFolder = folderIndex;
  }
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    requestFolder(folderIndex);
    m_bRevealCurrentFolder = true;
  }
  folderDragSource(folderIndex);
  acceptFolderDrop(folder.path);
  handleFolderContextMenu(folderIndex);
  if (m_selectedFolder == folderIndex) {
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                        ImGui::GetColorU32(ImGuiCol_HeaderActive),
                                        ImGui::GetStyle().FrameRounding, 0, 2.0f);
  }

  const ImVec2 buttonMin = ImGui::GetItemRectMin();
  const ImVec2 iconPos =
      ImVec2(buttonMin.x + (gridSize - 32) * 0.5f, buttonMin.y + (gridSize - 32) * 0.5f - 10);
  ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 32.0f, iconPos,
                                      IM_COL32(230, 190, 90, 255),
                                      UIHelpers::getFolderIcon(false));

  if (!renderFolderRename(folderIndex)) {
    ShortName shortName;
    const ANSICHAR* displayName = shortenName(folder.name.c_str(), shortName);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                         (gridSize - ImGui::CalcTextSize(displayName).x) * 0.5f);
    ImGui::TextUnformatted(displayName);
  }

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
  if (!renderFolderRename(folderIndex)) {
    if (ImGui::Selectable(folder.name.c_str(), m_selectedFolder == folderIndex,
                          ImGuiSelectableFlags_SpanAllColumns)) {
      m_selectedFolder = folderIndex;
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
      requestFolder(folderIndex);
      m_bRevealCurrentFolder = true;
    }
    folderDragSource(folderIndex);
    acceptFolderDrop(folder.path);
    handleFolderContextMenu(folderIndex);
  }

  ImGui::TableSetColumnIndex(2);
  ImGui::TextUnformatted("Folder");

  ImGui::PopID();
}

/*
 */
void
ContentAssetUI::handleFolderContextMenu(uint32 folderIndex)
{
  if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    ImGui::OpenPopup("FolderContext");
  }
  if (!ImGui::BeginPopup("FolderContext")) {
    return;
  }

  if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "  Open")) {
    requestFolder(folderIndex);
    m_bRevealCurrentFolder = true;
  }
  if (ImGui::MenuItem(ICON_FA_I_CURSOR "  Rename", "F2", false, folderIndex != 0)) {
    // The field is drawn in the folder's tile, so its parent must be the folder shown.
    const uint32 parent = m_folders[folderIndex].parent;
    if (parent != m_currentFolder) {
      requestFolder(parent);
      m_bRevealCurrentFolder = true;
    }
    startFolderRename(folderIndex);
    m_selectedFolder = folderIndex;
  }

  ImGui::Separator();
  // Like a single asset, a folder whose assets are loaded may still be in use.
  uint32 assetCount = 0;
  bool bInUse = false;
  const String& folderText = m_folders[folderIndex].path.toString();
  for (const SPtr<IAsset>& asset : m_assets) {
    if (isInsideFolder(asset->getAssetPath(), folderText)) {
      ++assetCount;
      bInUse = bInUse || !asset->isUnloaded();
    }
  }
  if (ImGui::MenuItem(ICON_FA_TRASH_ALT "  Delete", nullptr, false,
                      folderIndex != 0 && !bInUse)) {
    m_folderToDelete = m_folders[folderIndex].path;
    m_folderToDeleteAssets = assetCount;
    m_bOpenDeleteFolder = true;
  }
  if (bInUse) {
    ImGui::SetItemTooltip("An asset inside is loaded.");
  }
  ImGui::EndPopup();
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
  else if (asset->isTypeOf<ScriptAsset>()) {
    // Scripts are edited outside the editor; Play reads them from disk again.
    UIHelpers::openScript(asset->getFilePath());
  }
  else {
    AssetInspectorUI::open(asset);
    // A clip is watched more than read, so it also starts playing.
    if (asset->isTypeOf<AnimationClipAsset>()) {
      AnimationPreviewUI::open(asset);
    }
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
    m_selectedFolder = created;
    startFolderRename(created);
  }
}

/*
 */
void
ContentAssetUI::onAssetCreated(const SPtr<IAsset>& asset)
{
  refreshAssets();
  if (asset) {
    selectOnlyAsset(asset);
    startInlineRename(asset);
    m_bRenameRewritesScript = asset->isTypeOf<ScriptAsset>();
  }
}

/*
 */
SPtr<IAsset>
ContentAssetUI::createScript()
{
  AssetManager& assetManager = AssetManager::instance();
  const Path& folder = getCurrentFolderPath();
  const String name = assetManager.makeUniqueAssetName(folder, "NewScript");
  const SPtr<ScriptAsset> script = assetManager.createAsset<ScriptAsset>(name, folder).lock();
  if (!script) {
    CH_LOG_ERROR(ContentAssetUILog, "Failed to create a script.");
    return nullptr;
  }
  script->setSource(makeScriptTemplate(name));
  if (!assetManager.saveAsset(script)) {
    CH_LOG_ERROR(ContentAssetUILog, "Failed to save the script {0}.", name);
    return nullptr;
  }
  return script;
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
  if (ImGui::MenuItem(ICON_FA_IMAGES "  Regenerate Thumbnails")) {
    for (const uint32 index : m_visibleAssets) {
      AssetThumbnails::regenerate(*m_assets[index]);
    }
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
      if (material && assetManager.saveAsset(material)) {
        onAssetCreated(material);
      }
      else {
        CH_LOG_ERROR(ContentAssetUILog, "Failed to create a material.");
        refreshAssets();
      }
    }
    if (ImGui::MenuItem(ICON_FA_FILE_CODE "  Script")) {
      onAssetCreated(createScript());
    }
    ImGui::EndMenu();
  }

  ImGui::EndPopup();
}
} // namespace chEngineSDK
