/************************************************************************/
/**
 * @file chCommandLine.h
 * @author AccelMR
 * @date 2022/08/27
 * @brief Read-only access to the program's command line arguments.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {

/**
 * Holds the command line options, so any system can read them without them being passed
 * around. It is set once in main, before other threads start, and is read-only after
 * that, so it needs no locks and no start up or shut down.
 *
 * Options are "-Key=Value" or "-Flag" (any number of leading '-'). Keys and flags ignore
 * case; values keep it. Arguments without a leading '-' are ignored.
 *
 * Sample usage:
 * chEditor -GraphicsAPI=chVulkan -Width=1280 -NoSplash
 * CommandLine::getValue("graphicsapi", "chVulkan");
 * CommandLine::getInt("Width", 1920);
 * CommandLine::hasFlag("nosplash");
 */
class CH_UTILITY_EXPORT CommandLine
{
 public:
  /**
   * Replaces any options read before. argv is kept for getArgv, so it must live as long
   * as the program, as main's argv does.
   */
  static void
  initialize(int32 argc, const ANSICHAR* const* argv);

  NODISCARD static String
  getValue(const String& key, const String& defaultValue = "");

  /**
   * Returns defaultValue when the option is missing or its value is not a whole number.
   */
  NODISCARD static int32
  getInt(const String& key, int32 defaultValue = 0);

  NODISCARD static bool
  hasFlag(const String& flag);

  NODISCARD static int32
  getArgc() noexcept;

  NODISCARD static const ANSICHAR* const*
  getArgv() noexcept;
};

} // namespace chEngineSDK
