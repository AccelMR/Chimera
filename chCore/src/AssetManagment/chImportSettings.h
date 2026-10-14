/************************************************************************/
/**
 * @file chImportSettings.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  The options an asset was imported with, kept in its .meta file.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#if USING(CH_EDITOR)
#include "chModelExpand.h"

namespace chEngineSDK {

/**
 * Exists so whoever starts an import (the user, or a codec importing what a file uses) can
 * say how the data is meant to be used, and so a later import can repeat it. Each codec
 * reads only the fields it understands.
 */
struct ImportSettings
{
  // Textures: color data (base color, emissive) is stored as sRGB, so the GPU turns it into
  // linear values when it samples; normals and masks are data and stay linear.
  bool srgb = true;
  bool generateMips = true;

  // Models: how the model becomes GameObjects when it is dropped in a scene.
  ModelExpandMode expandMode = ModelExpandMode::Single;
  // Models: each mesh is cut into the pieces that do not touch, so props merged into one
  // mesh by the source file (all the pots of a level) can be moved one by one.
  bool splitParts = false;
  // Materials without an alpha mode in the file become masked when their base color
  // texture has pixels under the cutoff.
  bool autoAlphaMask = true;
};

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
