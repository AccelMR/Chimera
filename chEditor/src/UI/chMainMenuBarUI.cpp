/************************************************************************/
/**
 * @file chMainMenuBar.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chMainMenuBarUI.h"

#include "chAssetImporter.h"
#include "chAssetManagerImporter.h"
#include "chEnginePaths.h"
#include "chLogger.h"
#include "chMeshImporter.h"
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
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Render")) {
      ImGui::ColorEdit4("Background Color", UIHelpers::backgroundColor.toFloatPtr(),
                        ImGuiColorEditFlags_NoInputs);

      ImGui::Separator(); //--------------------------------------------------------------
      if (ImGui::ColorEdit4("Renderer Color", UIHelpers::rendererColor.toFloatPtr(),
                            ImGuiColorEditFlags_NoInputs)) {
        m_nastyRenderer->setClearColors({UIHelpers::rendererColor});
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
      if (ImGui::MenuItem("Import Model")) {
        auto importer = AssetManagerImporter::instance().getImporter<MeshImpotrter>();
        CH_ASSERT(importer && "MeshImpotrter importer must not be null.");

        Vector<String> supportedExtensions = importer->getSupportedExtensions();

        const Path filePath = UIHelpers::openFileExplorer(EnginePaths::getAbsoluteGameAssetDirectory(),
                                                          supportedExtensions);

        auto meshManager = AssetManagerImporter::instance().getImporter<MeshImpotrter>();
        auto importedModel =
          std::reinterpret_pointer_cast<ModelAsset>(meshManager->importAsset(filePath,
                                                    filePath.getFileName(false)));

        if (!importedModel) {
          CH_LOG_ERROR(MainMenuBarUILog, "Failed to import model: {0}", filePath.toString());
          ImGui::EndMenu();
          return;
        }

        m_nastyRenderer->loadModel(importedModel->getModel());
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

} // namespace chEngineSDK
