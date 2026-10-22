/************************************************************************/
/**
 * @file chScriptAllocator.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Memory of the script runtime.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Exists because scripts make many small short-lived objects (a temporary vector, a string, a
 * closure) and asking malloc for each one is slow. Blocks up to MAX_POOLED_SIZE come from
 * free lists of a few sizes, carved from pages that are kept until the allocator dies; bigger
 * blocks go to malloc. Main thread only, like the script runtime that owns it.
 */
class CH_CORE_EXPORT ScriptAllocator
{
 public:
  explicit ScriptAllocator(bool bUsePools = true) noexcept;

  ~ScriptAllocator();

  ScriptAllocator(const ScriptAllocator&) = delete;

  ScriptAllocator&
  operator=(const ScriptAllocator&) = delete;

  /**
   * A lua_Alloc: frees when newSize is 0, else (re)allocates to newSize. When pointer is null,
   * oldSize is the kind of object Lua makes, not a size. userData is the allocator.
   */
  static void*
  allocate(void* userData, void* pointer, SIZE_T oldSize, SIZE_T newSize);

  /**
   * Blocks asked for since the allocator was made (a resize that changes the block counts).
   */
  NODISCARD FORCEINLINE uint64
  getAllocationCount() const noexcept { return m_allocationCount; }

  NODISCARD FORCEINLINE SIZE_T
  getBytesInUse() const noexcept { return m_bytesInUse; }

  NODISCARD FORCEINLINE bool
  usesPools() const noexcept { return m_bUsePools; }

  static constexpr SIZE_T MAX_POOLED_SIZE = 256;

 private:
  struct FreeBlock
  {
    FreeBlock* next;
  };

  NODISCARD void*
  allocateBlock(SIZE_T size);

  void
  freeBlock(void* pointer, SIZE_T size) noexcept;

  NODISCARD void*
  resizeBlock(void* pointer, SIZE_T oldSize, SIZE_T newSize);

  NODISCARD void*
  takeFromPool(uint32 sizeClass);

  static constexpr uint32 SIZE_CLASS_COUNT = 12;
  static constexpr SIZE_T PAGE_SIZE = 64 * 1024;

  FreeBlock* m_freeLists[SIZE_CLASS_COUNT] = {};
  Vector<void*> m_pages;
  uint8* m_pageCursor = nullptr;
  uint8* m_pageEnd = nullptr;
  uint64 m_allocationCount = 0;
  SIZE_T m_bytesInUse = 0;
  bool m_bUsePools = true;
};

} // namespace chEngineSDK
