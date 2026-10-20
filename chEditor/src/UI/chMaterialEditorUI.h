/************************************************************************/
/**
 * @file chMaterialEditorUI.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief Edits the values and textures of a material asset.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class MaterialAsset;

/**
 * Exists so the Object Inspector (a model's material slot) and the Asset Inspector edit
 * materials with the same controls. Edits change the shared asset, so every object that
 * uses it follows, and the asset is saved once each edit ends.
 */
class MaterialEditorUI
{
 public:
  static void
  render(const SPtr<MaterialAsset>& materialAsset);
};

} // namespace chEngineSDK
