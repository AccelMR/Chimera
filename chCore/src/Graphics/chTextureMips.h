/************************************************************************/
/**
 * @file chTextureMips.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Builds the smaller mip levels of a texture on the CPU.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Exists so every importer builds mip chains the same way: a texture without mips shimmers
 * when it is drawn small, and sRGB colors must be averaged as linear light or the smaller
 * levels get darker.
 */
class CH_CORE_EXPORT TextureMips
{
 public:
  /**
   * Levels of a full chain down to 1x1.
   */
  NODISCARD static uint32
  getMipCount(uint32 width, uint32 height) noexcept;

  /**
   * Appends every level below mip 0 to pixels, which holds mip 0 as RGBA with 8 bits per
   * channel. Each level halves the size (rounding down, never below 1).
   *
   * @return the number of levels pixels holds afterwards.
   */
  static uint32
  appendChainRGBA8(Vector<uint8>& pixels, uint32 width, uint32 height, bool srgb);
};

} // namespace chEngineSDK
