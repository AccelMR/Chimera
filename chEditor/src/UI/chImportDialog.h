/************************************************************************/
/**
 * @file chImportDialog.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Asks for a file and the options to import it with.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chEventSystem.h"

namespace chEngineSDK {
class IAssetCodec;

/**
 * Exists so every import started by the user, from the main menu or the Content Browser,
 * asks for the same options before the codec runs, and so the panels that list assets hear
 * about the new one wherever the import started.
 */
class ImportDialog
{
 public:
  /**
   * Asks for a file with the codec's extensions, then shows its options; the import runs
   * when they are accepted. assetFolder is a virtual folder under /Game.
   */
  static void
  open(IAssetCodec& codec, const Path& assetFolder);

  /**
   * Draws the options while a file waits for them. Call once per frame, after the panels.
   */
  static void
  render();

  /**
   * The callback runs on the main thread with each asset the dialog imports.
   */
  NODISCARD static HEvent
  onAssetImported(const Function<void(const SPtr<IAsset>&)>& callback);
};

} // namespace chEngineSDK
