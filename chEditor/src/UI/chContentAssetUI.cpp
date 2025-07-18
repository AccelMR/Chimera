/************************************************************************/
/**
 * @file chContentAssetUI.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chContentAssetUI.h"

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
void
ContentAssetUI::refreshAssets() {
  m_assets = AssetManager::instance().getAllAssets();
}

/*
 */
void
ContentAssetUI::renderContentAssetUI() {
  renderDeleteConfirmationPopup();
  if (ImGui::Begin("Content Browser", &bShowContentWindow)) {
    // Search bar
    static char searchBuffer[256] = "";
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputTextWithHint("##search", "Search assets...", searchBuffer,
                                 sizeof(searchBuffer))) {
      // Filter will be applied below
    }

    ImGui::Separator();

    // Asset type filter buttons
    static bool showAllTypes = true;
    static bool showModels = true;
    static bool showTextures = true;
    static bool showMaterials = true;
    static bool showOther = true;

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

    // View mode toggle
    static bool gridView = true;
    if (ImGui::RadioButton("Grid View", gridView)) {
      gridView = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("List View", !gridView)) {
      gridView = false;
    }

    // Grid size slider for grid view
    static float gridSize = 80.0f;
    if (gridView) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100.0f);
      ImGui::SliderFloat("Size", &gridSize, 50.0f, 150.0f, "%.0f");
    }

    ImGui::Separator();

    // Asset display area
    ImGui::BeginChild("AssetArea", ImVec2(0, 0), false);

    // Convert search to lowercase for case-insensitive search
    String searchStr = String(searchBuffer);
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    if (gridView) {
      // Grid view implementation
      float windowWidth = ImGui::GetContentRegionAvail().x;
      int32 columns = static_cast<int32>(windowWidth / (gridSize + 10.0f));
      if (columns < 1) {
        columns = 1;
      }

      int32 currentColumn = 0;

      for (const auto& asset : m_assets) {
        // Filter by search
        if (!searchStr.empty()) {
          String assetName = asset->getName();
          std::transform(assetName.begin(), assetName.end(), assetName.begin(), ::tolower);
          if (assetName.find(searchStr) == String::npos) {
            continue;
          }
        }

        // Filter by asset type
        AssetIcon assetIcon = UIHelpers::getIconFromAssetType(asset);
        if (!showAllTypes) {
          bool shouldShow = false;
          switch (assetIcon.type) {
          case AssetType::Model:
            shouldShow = showModels;
            break;
          case AssetType::Texture:
            shouldShow = showTextures;
            break;
          case AssetType::Material:
            shouldShow = showMaterials;
            break;
          default:
            shouldShow = showOther;
            break;
          }
          if (!shouldShow) {
            continue;
          }
        }

        if (assetIcon.type == AssetType::Unknown) {
          continue; // Skip unknown asset types
        }

        // Position for grid layout
        if (currentColumn > 0) {
          ImGui::SameLine();
        }

        ImGui::PushID(asset->getUUID().toString().c_str());
        ImGui::BeginGroup();

        // Create unique ID for this asset
        String buttonId = chString::format("##asset_{0}", asset->getUUID().toString());

        // Asset icon button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

        if (ImGui::Button(buttonId.c_str(), ImVec2(gridSize, gridSize))) {
          // Asset selection logic
          handleAssetSelection(asset);
        }

        ImGui::PopStyleColor(3);

        // Draw icon on button
        ImVec2 buttonMin = ImGui::GetItemRectMin();
        ImVec2 buttonMax = ImGui::GetItemRectMax();
        ImVec2 iconPos = ImVec2(buttonMin.x + (gridSize - 32) * 0.5f,
                                buttonMin.y + (gridSize - 32) * 0.5f - 10);

        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 32.0f, iconPos,
                                            IM_COL32(255, 255, 255, 255), assetIcon.icon);

        // Asset name (truncated if too long)
        String displayName = asset->getName();
        if (displayName.length() > 12) {
          displayName = displayName.substr(0, 9) + "...";
        }

        if (!renderInlineRename(asset, displayName)) {
          // Normal text rendering
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

        // Asset state indicator
        ImVec4 stateColor = getAssetStateColor(asset);
        ImGui::GetWindowDrawList()->AddCircleFilled(
            ImVec2(buttonMax.x - 8, buttonMin.y + 8), 3.0f,
            ImGui::ColorConvertFloat4ToU32(stateColor));

        ImGui::EndGroup();

        // Make the group selectable for context menu
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
          ImGui::OpenPopup(
              chString::format("AssetContext_{0}", asset->getUUID().toString()).c_str());
        }

        // Context menu with explicit ID
        if (ImGui::BeginPopup(
                chString::format("AssetContext_{0}", asset->getUUID().toString()).c_str())) {
          renderAssetContextMenu(asset);
          ImGui::EndPopup();
        }

        ImGui::PopID(); // Pop the unique ID for this asset

        // Format createdAt (int64) as a readable date/time string
        auto createdAt = asset->getCreatedAt();

        // Convert from chrono ticks to time_t properly
        auto timePoint = std::chrono::system_clock::time_point(
            std::chrono::system_clock::duration(createdAt));
        std::time_t createdAtTimeT = std::chrono::system_clock::to_time_t(timePoint);

        ANSICHAR createdAtStr[64] = {};
        if (struct tm* timeInfo = std::localtime(&createdAtTimeT)) {
          std::strftime(createdAtStr, sizeof(createdAtStr), "%Y-%m-%d %H:%M:%S", timeInfo);
        }
        else {
          const String unkwon = "Unknown";
          chString::copyToANSI(createdAtStr, unkwon, unkwon.size());
        }

        // Tooltip
        if (ImGui::IsItemHovered()) {
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

        currentColumn = (currentColumn + 1) % columns;
      }
    }
    else {
      // List view implementation
      if (ImGui::BeginTable("AssetTable", 4,
                            ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
                                ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (const auto& asset : m_assets) {
          // Filter by search
          if (!searchStr.empty()) {
            String assetName = asset->getName();
            std::transform(assetName.begin(), assetName.end(), assetName.begin(), ::tolower);
            if (assetName.find(searchStr) == String::npos) {
              continue;
            }
          }

          // Filter by asset type
          AssetIcon assetIcon = UIHelpers::getIconFromAssetType(asset);
          if (!showAllTypes) {
            bool shouldShow = false;
            switch (assetIcon.type) {
            case AssetType::Model:
              shouldShow = showModels;
              break;
            case AssetType::Texture:
              shouldShow = showTextures;
              break;
            case AssetType::Material:
              shouldShow = showMaterials;
              break;
            default:
              shouldShow = showOther;
              break;
            }
            if (!shouldShow) {
              continue;
            }
          }

          if (assetIcon.type == AssetType::Unknown) {
            continue; // Skip unknown asset types
          }

          ImGui::TableNextRow();

          // Icon column
          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%s", assetIcon.icon);

          // Name column
          ImGui::TableSetColumnIndex(1);

          if (!renderInlineRename(asset, asset->getName())) {
            // Normal selectable item
            String selectableId = chString::format("{0}##asset_{1}", asset->getName(),
                                                   asset->getUUID().toString());
            if (ImGui::Selectable(selectableId.c_str(), false,
                                  ImGuiSelectableFlags_SpanAllColumns)) {
              handleAssetSelection(asset);
            }

            // Double-click to start rename
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
              startInlineRename(asset);
            }
          }

          // Context menu
          if (ImGui::BeginPopupContextItem()) {
            renderAssetContextMenu(asset);
            ImGui::EndPopup();
          }

          // Type column
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%s", asset->getTypeName());

          // State column
          ImGui::TableSetColumnIndex(3);
          ImVec4 stateColor = getAssetStateColor(asset);
          ImGui::TextColored(stateColor, "%s", getAssetStateString(asset).c_str());
        }

        ImGui::EndTable();
      }
    }

    ImGui::EndChild();
  }
  ImGui::End();
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
        const String assetWithExt(String(m_assetToDelete->getName()) + ".chAss");
        const Path fullAssetToDelete =
            EnginePaths::getAbsoluteGameAssetDirectory() / assetWithExt;
        const bool bRemovedCorrectly = FileSystem::removeFile(fullAssetToDelete);

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

  if (ImGui::MenuItem("Show in Explorer")) {
    // Implement file explorer opening logic
    CH_LOG_DEBUG(ContentAssetUILog, "Opening file explorer for: {0}", asset->getName());
  }

  if (ImGui::MenuItem("Properties")) {
    // Implement asset properties window
    CH_LOG_DEBUG(ContentAssetUILog, "Opening properties for: {0}", asset->getName());
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
    // Perform the actual rename
    bool success = AssetManager::instance().renameAsset(m_renamingAsset, newName.c_str());
    if (success) {
      CH_LOG_DEBUG(ContentAssetUILog, "Successfully renamed asset to: {0}", newName);
    }
    else {
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
ContentAssetUI::renderInlineRename(const SPtr<IAsset>& asset, const String& ) {
  // Check if this is the asset being renamed
  bool isThisAssetRenaming =
      m_isRenaming && m_renamingAsset && m_renamingAsset->getUUID() == asset->getUUID();

  if (isThisAssetRenaming) {
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
    bool enterPressed = ImGui::InputText(
        inputId.c_str(), m_renameBuffer, sizeof(m_renameBuffer),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

    // Handle input completion
    if (enterPressed || (!ImGui::IsItemActive() && !ImGui::IsItemFocused())) {
      if (enterPressed) {
        finishInlineRename();
      }
      else {
        cancelInlineRename();
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

  return false; // Normal text rendering should proceed
}

} // namespace chEngineSDK
