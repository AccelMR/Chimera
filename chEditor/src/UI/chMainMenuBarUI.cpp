/************************************************************************/
/**
 * @file chMainMenuBar.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chMainMenuBarUI.h"

#if USING(CH_IMPORTERS)
#include "chAssetImporter.h"
#include "chAssetImporterManager.h"
#endif // USING(CH_IMPORTERS)

#include "chAssetManager.h"
#include "chEnginePaths.h"
#include "chLogger.h"
#include "chPath.h"
#include "chModelAsset.h"
#include "chNastyRenderer.h"
#include "chUIHelpers.h"

#include "imgui.h"

namespace chEngineSDK {
using namespace chEngineSDK::chUIHelpers;

CH_LOG_DECLARE_STATIC(MainMenuBarUILog, All);

/*
 */
void
MainMenuBarUI::renderMainMenuBar() {
  if (!ImGui::BeginMainMenuBar()) {
    ImGui::EndMainMenuBar();
    return;
  }

  if (ImGui::BeginMenu("Render")) {
    ImGui::ColorEdit4("Background Color", UIHelpers::backgroundColor.toFloatPtr(),
                      ImGuiColorEditFlags_NoInputs);

    ImGui::Separator(); //--------------------------------------------------------------
    if (ImGui::ColorEdit4("Renderer Color", UIHelpers::rendererColor.toFloatPtr(),
                          ImGuiColorEditFlags_NoInputs)) {
      //m_multiStageRenderer->setClearColors({UIHelpers::rendererColor});
    }

    ImGui::Separator(); //--------------------------------------------------------------
    if (ImGui::SliderFloat("Font Size", &UIHelpers::baseFontSize, 1.0f, 5.0f, "%.1f",
                           ImGuiSliderFlags_AlwaysClamp)) {
      ImGui::GetStyle().FontScaleMain = UIHelpers::baseFontSize;
    }
    ImGui::Separator(); //--------------------------------------------------------------

    ImGui::MenuItem("Show ImGui Demo Window", nullptr, &UIHelpers::bShowDemoWindow);

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Asset")) {
    renderImportMenu();
    ImGui::EndMenu();
  }
  ImGui::EndMainMenuBar();
}

/*
 */
void
MainMenuBarUI::renderImportMenu() {
#if USING(CH_IMPORTERS)
  AssetImporterManager& importerManager = AssetImporterManager::instance();
  AssetManager& assetManager = AssetManager::instance();
  for (const auto& importer : importerManager.getAllImporters()) {
    for (const auto& assetType : importer->getSupportedAssetTypes()) {
      const String& importerTypeName = "Import " + assetManager.getAssetTypeName(assetType);
      if (ImGui::MenuItem(importerTypeName.c_str())) {
        const Path filePath = UIHelpers::openFileExplorer(
            EnginePaths::getAbsoluteGameAssetDirectory(), importer->getSupportedExtensions());

        if (filePath.empty()) {
          CH_LOG_ERROR(MainMenuBarUILog, "No file selected for import");
          ImGui::EndMenu();
          return;
        }

        auto importedAsset = importer->importAsset(filePath, filePath.getFileName(false));

        if (!importedAsset) {
          CH_LOG_ERROR(MainMenuBarUILog, "Failed to import asset: {0}", filePath.toString());
          ImGui::EndMenu();
          return;
        }
        CH_LOG_INFO(MainMenuBarUILog, "Successfully imported asset: {0} as {1}",
                    filePath.toString(), importedAsset->getUUID().toString());
        ImGui::EndMenu();
      }
    }
  }

#endif // USING(CH_IMPORTERS)
}

} // namespace chEngineSDK
