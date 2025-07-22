/************************************************************************/
/**
 * @file chEnginePaths.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 *  This file contains the paths used by the engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class CH_CORE_EXPORT EnginePaths {
 public:
  /**
   * Get the path to the assets directory. This is relative to the engine root.
   *
   * @return Path to the assets directory.
   */
  static Path
  getGameAssetDirectory();

  /**
   * Get the absolute path to the assets directory.
   *
   * @return Absolute path to the assets directory.
   */
  static Path
  getAbsoluteGameAssetDirectory();

  /**
   * Get the path to the engine assets directory. This is relative to the engine root.
   *
   * @return Path to the engine assets directory.
   */
  static Path
  getEngineAssetDirectory();

  /**
   * Get the absolute path to the engine assets directory.
   *
   * @return Absolute path to the engine assets directory.
   */
  static String
  getEngineAssetExtension();
};

} // namespace chEngineSDK
