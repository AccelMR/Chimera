/************************************************************************/
/**
 * @file chScriptAllocator.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Memory of the script runtime.
 */
/************************************************************************/
#include "chScriptAllocator.h"

#include <cstdlib>
#include <cstring>

#include "chMath.h"

namespace chEngineSDK {
namespace {
// Steps of 16 up to 128 (most Lua objects: strings, tables, closures, small userdata), then
// steps of 32. Every size is a multiple of 16, so every block keeps the 16-byte alignment of
// its page.
constexpr SIZE_T kClassSizes[] = {16, 32, 48, 64, 80, 96, 112, 128, 160, 192, 224, 256};

/*
 */
FORCEINLINE uint32
getSizeClass(const SIZE_T size) noexcept
{
  if (size <= 128) {
    return static_cast<uint32>((size + 15) / 16) - 1;
  }
  return 8 + static_cast<uint32>((size - 129) / 32);
}
} // namespace

static_assert(std::size(kClassSizes) == 12);
static_assert(kClassSizes[11] == ScriptAllocator::MAX_POOLED_SIZE);

/*
 */
ScriptAllocator::ScriptAllocator(const bool bUsePools) noexcept
  : m_bUsePools(bUsePools)
{}

/*
 */
ScriptAllocator::~ScriptAllocator()
{
  for (void* page : m_pages) {
    std::free(page);
  }
}

/*
 */
void*
ScriptAllocator::allocate(void* userData,
                          void* pointer,
                          const SIZE_T oldSize,
                          const SIZE_T newSize)
{
  ScriptAllocator& allocator = *static_cast<ScriptAllocator*>(userData);
  if (newSize == 0) {
    if (pointer) {
      allocator.freeBlock(pointer, oldSize);
    }
    return nullptr;
  }
  if (!pointer) {
    return allocator.allocateBlock(newSize);
  }
  return allocator.resizeBlock(pointer, oldSize, newSize);
}

/*
 */
void*
ScriptAllocator::allocateBlock(const SIZE_T size)
{
  void* const block = m_bUsePools && size <= MAX_POOLED_SIZE
                          ? takeFromPool(getSizeClass(size))
                          : std::malloc(size);
  if (block) {
    ++m_allocationCount;
    m_bytesInUse += size;
  }
  return block;
}

/*
 */
void
ScriptAllocator::freeBlock(void* pointer, const SIZE_T size) noexcept
{
  m_bytesInUse -= size;
  if (!m_bUsePools || size > MAX_POOLED_SIZE) {
    std::free(pointer);
    return;
  }
  const uint32 sizeClass = getSizeClass(size);
  FreeBlock* const block = static_cast<FreeBlock*>(pointer);
  block->next = m_freeLists[sizeClass];
  m_freeLists[sizeClass] = block;
}

/*
 */
void*
ScriptAllocator::resizeBlock(void* pointer, const SIZE_T oldSize, const SIZE_T newSize)
{
  const bool bOldPooled = m_bUsePools && oldSize <= MAX_POOLED_SIZE;
  const bool bNewPooled = m_bUsePools && newSize <= MAX_POOLED_SIZE;
  if (!bOldPooled && !bNewPooled) {
    void* const block = std::realloc(pointer, newSize);
    if (block) {
      m_bytesInUse = m_bytesInUse - oldSize + newSize;
    }
    return block;
  }
  if (bOldPooled && bNewPooled && getSizeClass(oldSize) == getSizeClass(newSize)) {
    m_bytesInUse = m_bytesInUse - oldSize + newSize;
    return pointer;
  }

  void* const block = allocateBlock(newSize);
  if (!block) {
    // Lua expects a shrink to never fail; the old block is big enough, so it stays.
    return newSize <= oldSize ? pointer : nullptr;
  }
  std::memcpy(block, pointer, Math::min(oldSize, newSize));
  freeBlock(pointer, oldSize);
  return block;
}

/*
 */
void*
ScriptAllocator::takeFromPool(const uint32 sizeClass)
{
  if (FreeBlock* const block = m_freeLists[sizeClass]) {
    m_freeLists[sizeClass] = block->next;
    return block;
  }

  const SIZE_T size = kClassSizes[sizeClass];
  if (static_cast<SIZE_T>(m_pageEnd - m_pageCursor) < size) {
    // What is left of the old page is smaller than the block and stays unused.
    uint8* const page = static_cast<uint8*>(std::malloc(PAGE_SIZE));
    if (!page) {
      return nullptr;
    }
    m_pages.push_back(page);
    m_pageCursor = page;
    m_pageEnd = page + PAGE_SIZE;
  }
  void* const block = m_pageCursor;
  m_pageCursor += size;
  return block;
}

} // namespace chEngineSDK
