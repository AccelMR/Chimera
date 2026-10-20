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
#include "chUUID.h"

namespace chEngineSDK {

/**
 * How a model with bones gets its skeleton. Fixed values, because the .meta file stores
 * them by name and old names must keep meaning the same.
 */
enum class ImportSkeletonMode : uint32
{
  // The first compatible skeleton of the project, else a new one.
  Auto = 0,
  CreateNew = 1,
  // ImportSettings::skeleton, or a new one when it does not fit.
  Existing = 2
};

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
  ImportSkeletonMode skeletonMode = ImportSkeletonMode::Auto;
  UUID skeleton = UUID::null();
  // Models: each animation of the file becomes an AnimationClip asset, its keys taken
  // animationSampleRate times per second.
  bool importAnimations = true;
  float animationSampleRate = 30.0f;
};

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
