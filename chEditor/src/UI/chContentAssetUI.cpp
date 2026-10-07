/************************************************************************/
/**
 * @file chContentAssetUI.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chContentAssetUI.h"

#include <chrono>
#include <cstring>
#include <ctime>

#if USING(CH_CODECS)
#include "chAssetCodec.h"
#include "chAssetCodecManager.h"
#endif // USING(CH_CODECS)

#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chLogger.h"
#include "chImGuiRenderer.h"
#include "chITexture.h"
#include "chEditorSelection.h"
#include "chMath.h"
#include "chUIHelpers.h"

#include "chGameObjectAsset.h"
#include "chTextureAsset.h"

#include "imgui.h"

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

} // namespace

/*
 */
ContentAssetUI::ContentAssetUI()
{
  refreshAssets();
}

/*
 */
void
ContentAssetUI::refreshAssets()
{
  m_assets = AssetManager::instance().getAllAssets();
  m_needsFilterUpdate = true;
}

/*
 */
void
ContentAssetUI::renderContentAssetUI()
{
  renderDeleteConfirmationPopup();

  if (!ImGui::Begin("Content Browser", &bShowContentWindow)) {
    ImGui::End();
    return;
  }

  renderSearchBar();
  renderAssetTypeFilters();
  renderViewModeControls();
  renderAssetDisplayArea();

  ImGui::End();
}

/*
 */
void
ContentAssetUI::saveUnsavedAssets()
{
  for (auto& weakAsset : m_unsavedAssets) {
    if (auto asset = weakAsset.lock()) {
      if (!AssetManager::instance().saveAsset(asset)) {
        CH_LOG_ERROR(ContentAssetUILog, "Failed to save asset: {0}", asset->getName());
      }
      else {
        CH_LOG_INFO(ContentAssetUILog, "Successfully saved asset: {0}", asset->getName());
      }
    }
  }
  m_unsavedAssets.clear();
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
  if (ImGui::Button("All")) {
    showAllTypes = true;
    showModels = showTextures = showMaterials = showOther = true;
    m_needsFilterUpdate = true;
  }
  ImGui::SameLine();

  if (ImGui::Button("Models")) {
    showAllTypes = false;
    showModels = true;
    showTextures = showMaterials = showOther = false;
    m_needsFilterUpdate = true;
  }
  ImGui::SameLine();

  if (ImGui::Button("Textures")) {
    showAllTypes = false;
    showTextures = true;
    showModels = showMaterials = showOther = false;
    m_needsFilterUpdate = true;
  }
  ImGui::SameLine();

  if (ImGui::Button("Materials")) {
    showAllTypes = false;
    showMaterials = true;
    showModels = showTextures = showOther = false;
    m_needsFilterUpdate = true;
  }
  ImGui::SameLine();

  if (ImGui::Button("Other")) {
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
  return passesSearchFilter(asset) && passesTypeFilter(asset);
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

  // The clipper works on rows of the grid, which all have the same height.
  const int32 visibleCount = static_cast<int32>(m_visibleAssets.size());
  const int32 rowCount = (visibleCount + columns - 1) / columns;

  ImGuiListClipper clipper;
  clipper.Begin(rowCount);
  while (clipper.Step()) {
    for (int32 row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
      const int32 first = row * columns;
      const int32 last = Math::min(first + columns, visibleCount);
      for (int32 i = first; i < last; ++i) {
        renderGridAssetItem(m_assets[m_visibleAssets[i]], i - first);
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

  ImGuiListClipper clipper;
  clipper.Begin(static_cast<int32>(m_visibleAssets.size()));
  while (clipper.Step()) {
    for (int32 i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
      renderListAssetItem(m_assets[m_visibleAssets[i]]);
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

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

  const uint64 thumbnail = assetIcon.type == AssetType::Texture ? getThumbnail(asset) : 0;

  if (thumbnail != 0) {
    ImGui::ImageButton("##asset", static_cast<ImTextureID>(thumbnail), buttonSize);
  }
  else {
    ImGui::Button("##asset", buttonSize);
  }
  AssetDragDrop::source(*asset);

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

  // Long names are cut so they fit under the icon.
  const ANSICHAR* displayName = asset->getName();
  ANSICHAR shortName[13];
  if (std::strlen(displayName) > 12) {
    std::memcpy(shortName, displayName, 9);
    std::memcpy(shortName + 9, "...", 4);
    displayName = shortName;
  }

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                       (gridSize - ImGui::CalcTextSize(displayName).x) * 0.5f);

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

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
  ImGui::Selectable(asset->getName(), false, ImGuiSelectableFlags_SpanAllColumns);
  AssetDragDrop::source(*asset);

  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    startInlineRename(asset);
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

  auto createdAt = asset->getCreatedAt();
  auto timePoint =
      std::chrono::system_clock::time_point(std::chrono::system_clock::duration(createdAt));
  std::time_t createdAtTimeT = std::chrono::system_clock::to_time_t(timePoint);

  // std::localtime returns a pointer to shared static data, so it is not thread-safe; the
  // platform versions fill a local struct instead.
  ANSICHAR createdAtStr[64] = {};
  struct tm timeInfo = {};
#if USING(CH_PLATFORM_WIN32)
  const bool hasLocalTime = localtime_s(&timeInfo, &createdAtTimeT) == 0;
#else
  const bool hasLocalTime = localtime_r(&createdAtTimeT, &timeInfo) != nullptr;
#endif
  if (hasLocalTime) {
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
  ImGui::Text("Imported Path: %s", asset->getImportedPath());
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
      texture ? ImGuiRenderer::getTextureId(texture->getBindlessIndex()) : 0;
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
        const String fullAssetToDelete =
            StringUtils::format("{0}/{1}.chAss", m_assetToDelete->getAssetPath(),
                                m_assetToDelete->getName());
        const bool bRemovedCorrectly = FileSystem::removeFile(Path(fullAssetToDelete));

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

  if (asset->isTypeOf<GameObjectAsset>()) {
    if (ImGui::MenuItem("Instantiate in Scene")) {
      CH_LOG_DEBUG(ContentAssetUILog, "Instantiating GameObject asset: {0}", asset->getName());
      ImGui::CloseCurrentPopup();
    }
    if (ImGui::MenuItem("Edit")) {
      SPtr<GameObjectAsset> gameObjectAsset = std::static_pointer_cast<GameObjectAsset>(asset);
      if (asset->isUnloaded()) {
        if (!AssetManager::instance().syncLoadAsset(asset)) {
          CH_LOG_ERROR(ContentAssetUILog, "Failed to load GameObject asset: {0}",
                       asset->getName());
          ImGui::CloseCurrentPopup();
          return;
        }
      }
      CH_LOG_DEBUG(ContentAssetUILog, "Editing GameObject asset: {0}", asset->getName());
      EditorSelection::setGameObjectAssetPreview(gameObjectAsset->getGameObject());
      ImGui::CloseCurrentPopup();
    }
    ImGui::Separator();
  }

  if (ImGui::MenuItem("Load")) {
    loadAsset(asset);
  }

  if (ImGui::MenuItem("Unload", nullptr, false, asset->isLoaded())) {
    CH_LOG_DEBUG(ContentAssetUILog, "Unloading asset: {0}", asset->getName());
    AssetManager::instance().unloadAsset(asset->getUUID());
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Rename")) {
    startInlineRename(asset);
    ImGui::CloseCurrentPopup();
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Delete", nullptr, false, asset->isUnloaded())) {
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
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.4f, 0.4f, 0.4f, 0.9f));

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

  ImGui::PopStyleColor(3);
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
ContentAssetUI::renderEmptyAreaContextMenu()
{
  if (!ImGui::BeginPopup("EmptyAreaContextMenu")) {
    return;
  }

  if (ImGui::MenuItem("Refresh")) {
    refreshAssets();
    CH_LOG_INFO(ContentAssetUILog, "Refreshed asset list.");
  }

  ImGui::Separator();

  if (ImGui::BeginMenu("Import Asset")) {
    ImGui::Separator();
#if USING(CH_CODECS)
    AssetCodecManager& codecManager = AssetCodecManager::instance();
    AssetManager& assetManager = AssetManager::instance();
    for (const auto& codec : codecManager.getAllCodecs()) {
      for (const auto& assetType : codec->getSupportedAssetTypes()) {
        const String& codecTypeName = assetManager.getAssetTypeName(assetType);
        if (ImGui::MenuItem(codecTypeName.c_str())) {
          if (UIHelpers::importAssetWithDialog(codec)) {
            refreshAssets();
          }
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

  if (ImGui::BeginMenu("Create")) {
    if (ImGui::MenuItem("Game Object Asset")) {
      WeakPtr<IAsset> newAsset = AssetManager::instance().createAsset<GameObjectAsset>(
          "New", EnginePaths::getGameAssetDirectory());
      if (newAsset.expired()) {
        CH_LOG_ERROR(ContentAssetUILog, "Failed to create Game Object Asset.");
      }
      else {
        m_unsavedAssets.push_back(newAsset);
        refreshAssets();
      }
    }
    ImGui::EndMenu();
  }

  if (ImGui::MenuItem("Save All Unsaved Assets")) {
    saveUnsavedAssets();
  }

  ImGui::EndPopup();
}
} // namespace chEngineSDK
