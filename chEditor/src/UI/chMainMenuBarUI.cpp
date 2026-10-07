/************************************************************************/
/**
 * @file chMainMenuBar.cpp
 * @author AccelMR
 * @date 2025/07/17
 * @brief
 */
/************************************************************************/
#include "chMainMenuBarUI.h"

#if USING(CH_CODECS)
#include "chAssetCodec.h"
#include "chAssetCodecManager.h"
#endif // USING(CH_CODECS)

#include "chAssetManager.h"
#include "chLinearColor.h"
#include "chLogger.h"
#include "chModelAsset.h"
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
    // The scene renderer reads the color every frame.
    ImGui::ColorEdit4("Renderer Color", UIHelpers::rendererColor.toFloatPtr(),
                      ImGuiColorEditFlags_NoInputs);

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
MainMenuBarUI::renderImportMenu()
{
#if USING(CH_CODECS)
  if (!ImGui::BeginMenu("Import")) {
    return;
  }

  AssetManager& assetManager = AssetManager::instance();
  for (const auto& codec : AssetCodecManager::instance().getAllCodecs()) {
    for (const auto& assetType : codec->getSupportedAssetTypes()) {
      const String& typeName = assetManager.getAssetTypeName(assetType);
      if (ImGui::MenuItem(typeName.c_str())) {
        UIHelpers::importAssetWithDialog(codec);
      }
    }
  }

  ImGui::EndMenu();
#endif // USING(CH_CODECS)
}

} // namespace chEngineSDK
