/************************************************************************/
/**
 * @file chContentAssetUI.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chContentAssetUI.h"

#if USING(CH_IMPORTERS)
#include "chAssetImporter.h"
#include "chAssetImporterManager.h"
#endif // USING(CH_IMPORTERS)

#include "chAssetManager.h"
#include "chLogger.h"
#include "chMath.h"
#include "chUIHelpers.h"

#include "chNastyRenderer.h"

#include "imgui.h"

#if USING(CH_DISPLAY_SDL3)

#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#endif // USING(CH_DISPLAY_SDL3)

namespace chEngineSDK {
using namespace chEngineSDK::chUIHelpers;

CH_LOG_DECLARE_STATIC(ContentAssetUILog, All);

namespace ContentAssetUIVars {
// Variable to control the visibility of the file explorer
static bool bShowContentWindow = false;
} // namespace ContentAssetUIVars
using namespace ContentAssetUIVars;

/*
*/
ContentAssetUI::ContentAssetUI() {
  AssetManager::instance().onAssetsChanged(
      [this](const Vector<SPtr<IAsset>>& assets) {
        m_assets = assets;
      });
  m_assets = AssetManager::instance().getAllAssets();
}

// Main render function - now much cleaner
void
ContentAssetUI::renderContentAssetUI() {
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

// Search bar rendering
void
ContentAssetUI::renderSearchBar() {
  ImGui::SetNextItemWidth(-1.0f);

  if (ImGui::InputTextWithHint("##search", "Search assets...", searchBuffer,
                               sizeof(searchBuffer))) {
    // Filter will be applied in display area
  }

  ImGui::Separator();
}

// Asset type filter buttons
void
ContentAssetUI::renderAssetTypeFilters() {
  if (ImGui::Button("All")) {
    showAllTypes = true;
    showModels = showTextures = showMaterials = showOther = true;
  }
  ImGui::SameLine();

  if (ImGui::Button("Models")) {
    showAllTypes = false;
    showModels = true;
    showTextures = showMaterials = showOther = false;
  }
  ImGui::SameLine();

  if (ImGui::Button("Textures")) {
    showAllTypes = false;
    showTextures = true;
    showModels = showMaterials = showOther = false;
  }
  ImGui::SameLine();

  if (ImGui::Button("Materials")) {
    showAllTypes = false;
    showMaterials = true;
    showModels = showTextures = showOther = false;
  }
  ImGui::SameLine();

  if (ImGui::Button("Other")) {
    showAllTypes = false;
    showOther = true;
    showModels = showTextures = showMaterials = false;
  }

  ImGui::Separator();
}

// View mode and grid size controls
void
ContentAssetUI::renderViewModeControls() {
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
    ImGui::SliderFloat("Size", &gridSize, 50.0f, 150.0f, "%.0f");
  }

  ImGui::Separator();
}

// Main asset display area
void
ContentAssetUI::renderAssetDisplayArea() {
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

// Grid view implementation
void
ContentAssetUI::renderGridView() {

  float windowWidth = ImGui::GetContentRegionAvail().x;
  int32 columns = static_cast<int32>(windowWidth / (gridSize + 10.0f));
  if (columns < 1) {
    columns = 1;
  }

  int32 currentColumn = 0;

  for (const auto& asset : m_assets) {
    if (!shouldShowAsset(asset)) {
      continue;
    }

    renderGridAssetItem(asset, currentColumn, gridSize);
    currentColumn = (currentColumn + 1) % columns;
  }
}

// List view implementation
void
ContentAssetUI::renderListView() {
  if (!ImGui::BeginTable("AssetTable", 4,
                         ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
                             ImGuiTableFlags_Borders)) {
    return;
  }

  setupTableColumns();

  for (const auto& asset : m_assets) {
    if (!shouldShowAsset(asset)) {
      continue;
    }

    renderListAssetItem(asset);
  }

  ImGui::EndTable();
}

// Check if asset should be shown based on filters
bool
ContentAssetUI::shouldShowAsset(const SPtr<IAsset>& asset) {
  // Search filter
  if (!passesSearchFilter(asset)) {
    return false;
  }

  // Type filter
  if (!passesTypeFilter(asset)) {
    return false;
  }

  // Skip unknown asset types
  AssetIcon assetIcon = UIHelpers::getIconFromAssetType(asset);
  if (assetIcon.type == AssetType::Unknown) {
    return false;
  }

  return true;
}

// Check search filter
bool
ContentAssetUI::passesSearchFilter(const SPtr<IAsset>& asset) {
  String searchStr = String(searchBuffer);

  if (searchStr.empty()) {
    return true;
  }

  std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

  String assetName = asset->getName();
  std::transform(assetName.begin(), assetName.end(), assetName.begin(), ::tolower);

  return assetName.find(searchStr) != String::npos;
}

// Check type filter
bool
ContentAssetUI::passesTypeFilter(const SPtr<IAsset>& asset) {
  if (showAllTypes) {
    return true;
  }

  AssetIcon assetIcon = UIHelpers::getIconFromAssetType(asset);

  switch (assetIcon.type) {
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

// Setup table columns for list view
void
ContentAssetUI::setupTableColumns() {
  ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 40.0f);
  ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
  ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
  ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 80.0f);
  ImGui::TableHeadersRow();
}

// Render single asset item in grid view
void
ContentAssetUI::renderGridAssetItem(const SPtr<IAsset>& asset, int32 currentColumn,
                                    float gridSize) {
  if (currentColumn > 0) {
    ImGui::SameLine();
  }

  ImGui::PushID(asset->getUUID().toString().c_str());
  ImGui::BeginGroup();

  renderAssetIconButton(asset, gridSize);
  renderAssetNameInGrid(asset, gridSize);

  ImGui::EndGroup();

  renderAssetStateIndicator(asset);
  handleAssetContextMenu(asset);
  renderAssetTooltip(asset);

  ImGui::PopID();
}

// Render single asset item in list view
void
ContentAssetUI::renderListAssetItem(const SPtr<IAsset>& asset) {
  ImGui::TableNextRow();

  // Icon column
  ImGui::TableSetColumnIndex(0);
  AssetIcon assetIcon = UIHelpers::getIconFromAssetType(asset);
  ImGui::Text("%s", assetIcon.icon);

  // Name column
  ImGui::TableSetColumnIndex(1);
  if (!renderInlineRename(asset, asset->getName())) {
    renderSelectableAssetName(asset);
  }

  handleAssetContextMenu(asset);

  // Type column
  ImGui::TableSetColumnIndex(2);
  ImGui::Text("%s", asset->getTypeName());

  // State column
  ImGui::TableSetColumnIndex(3);
  ImVec4 stateColor = getAssetStateColor(asset);
  ImGui::TextColored(stateColor, "%s", getAssetStateString(asset).c_str());
}

// Render asset icon button for grid view
void
ContentAssetUI::renderAssetIconButton(const SPtr<IAsset>& asset, float gridSize) {
  String buttonId = chString::format("##asset_{0}", asset->getUUID().toString());
  AssetIcon assetIcon = UIHelpers::getIconFromAssetType(asset);

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

  if (ImGui::Button(buttonId.c_str(), ImVec2(gridSize, gridSize))) {
    handleAssetSelection(asset);
  }

  ImGui::PopStyleColor(3);

  // Draw icon on button
  ImVec2 buttonMin = ImGui::GetItemRectMin();
  ImVec2 iconPos =
      ImVec2(buttonMin.x + (gridSize - 32) * 0.5f, buttonMin.y + (gridSize - 32) * 0.5f - 10);

  ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 32.0f, iconPos,
                                      IM_COL32(255, 255, 255, 255), assetIcon.icon);
}

// Render asset name in grid view
void
ContentAssetUI::renderAssetNameInGrid(const SPtr<IAsset>& asset, float gridSize) {
  String displayName = asset->getName();
  if (displayName.length() > 12) {
    displayName = displayName.substr(0, 9) + "...";
  }

  if (renderInlineRename(asset, displayName)) {
    return;
  }

  // Center the text
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                       (gridSize - ImGui::CalcTextSize(displayName.c_str()).x) * 0.5f);

  // Make the text clickable for rename
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // Transparent
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

  String textButtonId =
      chString::format("{0}##text_{1}", displayName, asset->getUUID().toString());

  if (ImGui::Button(textButtonId.c_str())) {
    // Single click - could be used for selection or other actions
  }

  // Double-click to start rename
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    startInlineRename(asset);
  }

  ImGui::PopStyleColor(3);
}

// Render selectable asset name for list view
void
ContentAssetUI::renderSelectableAssetName(const SPtr<IAsset>& asset) {
  String selectableId =
      chString::format("{0}##asset_{1}", asset->getName(), asset->getUUID().toString());

  if (ImGui::Selectable(selectableId.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
    handleAssetSelection(asset);
  }

  // Double-click to start rename
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    startInlineRename(asset);
  }
}

// Render asset state indicator
void
ContentAssetUI::renderAssetStateIndicator(const SPtr<IAsset>& asset) {
  const ImVec4 stateColor = getAssetStateColor(asset);
  const ImVec2 groupMin = ImGui::GetItemRectMin();
  const ImVec2 groupMax = ImGui::GetItemRectMax();

  // Proporciones basadas en gridSize
  const float offsetRatio = 0.08f; // 8% del tamaño del grid para el offset
  const float radiusRatio = 0.05f; // 5% del tamaño del grid para el radio

  float offset = gridSize * offsetRatio;
  float radius = gridSize * radiusRatio;

  // Límites mínimos y máximos para que no se vea muy pequeño o muy grande
  offset = Math::max(6.0f, Math::min(offset, 15.0f));
  radius = Math::max(2.0f, Math::min(radius, 8.0f));

  ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(groupMax.x - offset, groupMin.y + offset),
                                              radius,
                                              ImGui::ColorConvertFloat4ToU32(stateColor));
}

// Handle asset context menu
void
ContentAssetUI::handleAssetContextMenu(const SPtr<IAsset>& asset) {
  bool groupRightClicked =
      ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
  String popupId = chString::format("AssetContext_{0}", asset->getUUID().toString());

  if (groupRightClicked) {
    ImGui::OpenPopup(popupId.c_str());
  }

  if (ImGui::BeginPopup(popupId.c_str())) {
    renderAssetContextMenu(asset);
    ImGui::EndPopup();
  }
}

// Render asset tooltip
void
ContentAssetUI::renderAssetTooltip(const SPtr<IAsset>& asset) {
  if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
    return;
  }

  // Format createdAt (int64) as a readable date/time string
  auto createdAt = asset->getCreatedAt();
  auto timePoint =
      std::chrono::system_clock::time_point(std::chrono::system_clock::duration(createdAt));
  std::time_t createdAtTimeT = std::chrono::system_clock::to_time_t(timePoint);

  ANSICHAR createdAtStr[64] = {};
  if (struct tm* timeInfo = std::localtime(&createdAtTimeT)) {
    std::strftime(createdAtStr, sizeof(createdAtStr), "%Y-%m-%d %H:%M:%S", timeInfo);
  }
  else {
    const String unknown = "Unknown";
    chString::copyToANSI(createdAtStr, unknown, unknown.size());
  }

  ImGui::BeginTooltip();
  ImGui::Text("UUID: %s", asset->getUUID().toString().c_str());
  ImGui::Text("Name: %s", asset->getName());
  ImGui::Text("Type: %s", asset->getTypeName());
  ImGui::Text("Created At: %s", createdAtStr);
  ImGui::Text("State: %s", getAssetStateString(asset).c_str());
  ImGui::Text("Imported Path: %s", asset->getImportedPath());
  ImGui::Text("Asset Path: %s", asset->getAssetPath());
  ImGui::EndTooltip();
}

/*
 */
void
ContentAssetUI::renderDeleteConfirmationPopup() {
  if (!m_showDeleteConfirmation || !m_assetToDelete) {
    m_showDeleteConfirmation = false;
    return; // No asset to delete or confirmation not needed
  }
  // Render the delete confirmation popup
  ImGui::OpenPopup("Delete Asset?");

  if (ImGui::BeginPopupModal("Delete Asset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    if (m_assetToDelete) {
      ImGui::Text("Are you sure you want to delete this asset?");
      ImGui::Separator();
      ImGui::Text("Name: %s", m_assetToDelete->getName());
      ImGui::Text("Type: %s", m_assetToDelete->getTypeName());
      ImGui::Separator();
      ImGui::Text("This action cannot be undone!");

      // Buttons
      if (ImGui::Button("Delete", ImVec2(120, 0))) {
        // Perform deletion
        const String fullAssetToDelete =
          chString::format("{0}/{1}.chAss", m_assetToDelete->getAssetPath(),
                                            m_assetToDelete->getName());
        const bool bRemovedCorrectly = FileSystem::removeFile(Path(fullAssetToDelete));

        if (bRemovedCorrectly) {
          AssetManager::instance().removeAsset(m_assetToDelete->getUUID());
          m_assets = AssetManager::instance().getAllAssets();
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
ContentAssetUI::handleAssetSelection(const SPtr<IAsset>& asset) {
  CH_LOG_DEBUG(ContentAssetUILog, "Selected asset: {0}", asset->getName());

  if (AssetManager::instance().loadAsset(asset)) {
    CH_LOG_DEBUG(ContentAssetUILog, "Loading asset: {0}", asset->getName());

    // Handle different asset types
    if (asset->isTypeOf<ModelAsset>()) {
      m_nastyRenderer->loadModel(asset->as<ModelAsset>()->getModel());
      CH_LOG_DEBUG(ContentAssetUILog, "Loaded model asset: {0}", asset->getName());
    }
    // Add more asset type handling here as needed
    // else if (asset->isTypeOf<TextureAsset>()) { ... }
    // else if (asset->isTypeOf<MaterialAsset>()) { ... }
  }
  else {
    CH_LOG_ERROR(ContentAssetUILog, "Failed to load asset: {0}", asset->getName());
  }
}

/*
 */
void
ContentAssetUI::renderAssetContextMenu(const SPtr<IAsset>& asset) {
  if (ImGui::MenuItem("Load")) {
    handleAssetSelection(asset);
  }

  if (ImGui::MenuItem("Unload", nullptr, false, asset->isLoaded())) {
    CH_LOG_DEBUG(ContentAssetUILog, "Unloading asset: {0}", asset->getName());
    AssetManager::instance().unloadAsset(asset->getUUID());
    m_nastyRenderer->loadModel(nullptr);
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Rename")) {
    startInlineRename(asset);
    ImGui::CloseCurrentPopup();
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Delete", nullptr, false, asset->isUnloaded())) {
    // Set the asset to delete and open the confirmation popup
    m_assetToDelete = asset;
    m_showDeleteConfirmation = true;

    ImGui::CloseCurrentPopup();
  }
}

/*
 */
ImVec4
ContentAssetUI::getAssetStateColor(const SPtr<IAsset>& asset) {
  switch (asset->getState()) {
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

/*
 */
String
ContentAssetUI::getAssetStateString(const SPtr<IAsset>& asset) {
  switch (asset->getState()) {
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

/*
 */
void
ContentAssetUI::startInlineRename(const SPtr<IAsset>& asset) {
  if (!asset) {
    return;
  }

  m_isRenaming = true;
  m_renamingAsset = asset;
  m_renameFocusRequested = true;

  // Copy current name to buffer
  chString::copyANSI(m_renameBuffer, asset->getName());

  CH_LOG_DEBUG(ContentAssetUILog, "Started inline rename for asset: {0}", asset->getName());
}

/*
 */
void
ContentAssetUI::finishInlineRename() {
  if (!m_isRenaming || !m_renamingAsset) {
    return;
  }

  String newName = String(m_renameBuffer);
  newName = chString::trim(newName); // Remove leading/trailing whitespace

  if (!newName.empty() && newName != m_renamingAsset->getName()) {
    if (!AssetManager::instance().renameAsset(m_renamingAsset, newName.c_str())) {
      CH_LOG_ERROR(ContentAssetUILog, "Failed to rename asset to: {0}", newName);
    }
  }

  cancelInlineRename();
}

/*
 */
void
ContentAssetUI::cancelInlineRename() {
  m_isRenaming = false;
  m_renamingAsset = nullptr;
  m_renameFocusRequested = false;
  memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
}

/*
 */
bool
ContentAssetUI::renderInlineRename(const SPtr<IAsset>& asset, const String&) {
  // Check if this is the asset being renamed
  const bool isThisAssetRenaming =
      m_isRenaming && m_renamingAsset && m_renamingAsset->getUUID() == asset->getUUID();

  if (!isThisAssetRenaming) {
    return false; // Normal text rendering should proceed
  }
  // Calculate text width for proper sizing
  ImVec2 textSize = ImGui::CalcTextSize(m_renameBuffer);
  float inputWidth = Math::max(textSize.x + 20.0f, 100.0f); // Minimum width of 100px

  // Style the input to look more like regular text
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.4f, 0.4f, 0.4f, 0.9f));

  ImGui::SetNextItemWidth(inputWidth);

  // Create unique ID for this input
  String inputId = chString::format("##rename_{0}", asset->getUUID().toString());

  // Handle focus request
  if (m_renameFocusRequested) {
    ImGui::SetKeyboardFocusHere();
    m_renameFocusRequested = false;
  }

  // Render the input field
  bool enterPressed = ImGui::InputText(inputId.c_str(), m_renameBuffer, sizeof(m_renameBuffer),
                                       ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_AutoSelectAll);

  // Handle input completion
  if (enterPressed) {
    finishInlineRename();
  }
  else if (ImGui::IsItemDeactivated()) {
    // Input lost focus - check if escape was pressed
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      cancelInlineRename();
    }
    else {
      finishInlineRename(); // Accept the change
    }
  }

  // Handle escape key to cancel
  if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    cancelInlineRename();
  }

  ImGui::PopStyleColor(3);
  ImGui::PopStyleVar(2);

  return true; // Indicate that rename widget was rendered
}

/*
 */
void
ContentAssetUI::handleEmptyAreaContextMenu() {
  // Early return if any item is hovered
  if (ImGui::IsAnyItemHovered()) {
    return;
  }

  // Early return if not hovering window
  if (!ImGui::IsWindowHovered()) {
    return;
  }

  // Check for right click in empty area
  bool emptyAreaRightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

  if (!emptyAreaRightClicked) {
    return;
  }

  ImGui::OpenPopup("EmptyAreaContextMenu");
}

/*
 */
void
ContentAssetUI::renderEmptyAreaContextMenu() {
  if (!ImGui::BeginPopup("EmptyAreaContextMenu")) {
    return;
  }

  // Make a menu that hass submenu importers
  if (ImGui::BeginMenu("Import Asset")) {
    ImGui::Separator();
#if USING(CH_IMPORTERS)
    AssetImporterManager& importerManager = AssetImporterManager::instance();
    AssetManager& assetManager = AssetManager::instance();
    for (const auto& importer : importerManager.getAllImporters()) {
      for (const auto& assetType : importer->getSupportedAssetTypes()) {
        const String& importerTypeName = assetManager.getAssetTypeName(assetType);
        if (ImGui::MenuItem(importerTypeName.c_str())) {
          const Path filePath =
              UIHelpers::openFileExplorer(EnginePaths::getAbsoluteGameAssetDirectory(),
                                          importer->getSupportedExtensions());

          if (filePath.empty()) {
            CH_LOG_ERROR(ContentAssetUILog, "No file selected for import");
          ImGui::EndMenu();
          ImGui::EndPopup();
          return; // Exit after handling import
          }

          auto importedAsset = importer->importAsset(filePath, filePath.getFileName(false));

          if (!importedAsset) {
            CH_LOG_ERROR(ContentAssetUILog, "Failed to import asset: {0}",
                         filePath.toString());
          ImGui::EndMenu();
          ImGui::EndPopup();
          return; // Exit after handling import
          }

          CH_LOG_INFO(ContentAssetUILog, "Successfully imported asset: {0} as {1}",
                      filePath.toString(), importedAsset->getUUID().toString());
          ImGui::EndMenu();
          ImGui::EndPopup();
          return; // Exit after handling import
        }
      }
    }
#endif // USING(CH_IMPORTERS)
    ImGui::EndMenu();
  }
  ImGui::EndPopup();
}
} // namespace chEngineSDK
