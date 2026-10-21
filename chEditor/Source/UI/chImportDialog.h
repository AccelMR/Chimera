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
 * asks for the same options before the codec runs, reads the file on a worker thread so the
 * editor keeps drawing, and lets the panels that list assets hear about the new one
 * wherever the import started.
 */
class ImportDialog
{
 public:
  /**
   * Asks for a file with the codec's extensions, then shows its options; the import starts
   * when they are accepted. assetFolder is a virtual folder under /Game. Returns at once:
   * the file dialog answers in a later frame. Ignored while another import is open.
   */
  static void
  open(IAssetCodec& codec, const Path& assetFolder);

  /**
   * Draws the options while a file waits for them and the progress of a running import.
   * Call once per frame, after the panels.
   */
  static void
  render();

  /**
   * Makes the assets once the worker has read the file. Call once per frame, also while the
   * UI is hidden.
   */
  static void
  update();

  /**
   * Cancels a running import and waits for its worker. Call before the codecs unload.
   */
  static void
  shutDown();

  /**
   * The callback runs on the main thread with each asset the dialog imports.
   */
  NODISCARD static HEvent
  onAssetImported(const Function<void(const SPtr<IAsset>&)>& callback);
};

} // namespace chEngineSDK
