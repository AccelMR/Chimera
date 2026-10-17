/************************************************************************/
/**
 * @file chImportTask.h
 * @author AccelMR
 * @date 2026/10/16
 * @brief
 *  One import split into a part that reads the source and a part that makes the assets.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#if USING(CH_EDITOR)
#include "chSTDThreading.h"

namespace chEngineSDK {

/**
 * Exists so the thread that runs an import and the one that shows it can share how far it
 * got and a request to stop, without a lock.
 */
class ImportProgress
{
 public:
  /**
   * fraction goes from 0 to 1; a negative value means the import cannot tell. label must be
   * a string literal: only the pointer is kept.
   */
  void
  set(const ANSICHAR* label, float fraction) noexcept
  {
    m_label.store(label, std::memory_order_relaxed);
    m_fraction.store(fraction, std::memory_order_relaxed);
  }

  void
  setFraction(float fraction) noexcept
  {
    m_fraction.store(fraction, std::memory_order_relaxed);
  }

  NODISCARD const ANSICHAR*
  getLabel() const noexcept
  {
    return m_label.load(std::memory_order_relaxed);
  }

  NODISCARD float
  getFraction() const noexcept
  {
    return m_fraction.load(std::memory_order_relaxed);
  }

  void
  cancel() noexcept
  {
    m_bCancelled.store(true, std::memory_order_relaxed);
  }

  NODISCARD bool
  isCancelled() const noexcept
  {
    return m_bCancelled.load(std::memory_order_relaxed);
  }

 private:
  Atomic<const ANSICHAR*> m_label{"Reading"};
  Atomic<float> m_fraction{-1.0f};
  Atomic<bool> m_bCancelled{false};
};

/**
 * Exists so a long import can read and convert its source on a worker thread while the
 * editor keeps drawing, and only make, save and register the assets on the main thread,
 * where AssetManager and the graphics API live. Made by IAssetCodec::createImportTask.
 * Tasks run code of the plugin that made them, so they must be destroyed before it unloads.
 */
class CH_CORE_EXPORT IImportTask
{
 public:
  virtual ~IImportTask() = default;

  /**
   * Any thread. Reads the source into CPU data; must not touch AssetManager, the graphics
   * API or a scene. Returns false when the source cannot be read or progress was cancelled.
   */
  NODISCARD virtual bool
  readSource(ImportProgress& progress) = 0;

  /**
   * Main thread, after readSource succeeded. Makes the assets in assetFolder (a virtual
   * path under /Game), saves and registers them, and returns the main one.
   */
  virtual SPtr<IAsset>
  createAssets(const Path& assetFolder) = 0;
};

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
