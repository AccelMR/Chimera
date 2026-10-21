/************************************************************************/
/**
 * @file chLinearAllocator.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Hands out memory by moving an offset, and frees all of it at once.
 */
/************************************************************************/
#pragma once

#include <cstddef>

#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {

/**
 * Exists for data that lives for one frame (or another short scope) and is rebuilt every
 * time: an allocation moves an offset inside a block, and reset() frees everything at once
 * while keeping the blocks, so after the first frames nothing goes to the heap.
 *
 * Objects made with create() that have a destructor get it called by reset(), newest first.
 * Memory from allocate() is never constructed or destroyed. Not thread-safe.
 */
class CH_UTILITY_EXPORT LinearAllocator
{
 public:
  explicit LinearAllocator(SIZE_T blockSize = 64 * 1024);

  ~LinearAllocator();

  LinearAllocator(const LinearAllocator&) = delete;

  LinearAllocator&
  operator=(const LinearAllocator&) = delete;

  /**
   * alignment must be a power of two. A size bigger than the block size gets a block of
   * its own, kept like the others.
   */
  NODISCARD FORCEINLINE void*
  allocate(SIZE_T size, SIZE_T alignment = alignof(std::max_align_t))
  {
    if (m_currentBlock < m_blocks.size()) {
      const Block& block = m_blocks[m_currentBlock];
      const SIZE_T address = reinterpret_cast<SIZE_T>(block.data) + m_offset;
      const SIZE_T aligned = (address + alignment - 1) & ~(alignment - 1);
      const SIZE_T end = aligned - reinterpret_cast<SIZE_T>(block.data) + size;
      if (end <= block.size) {
        m_offset = end;
        return reinterpret_cast<void*>(aligned);
      }
    }
    return allocateFromNextBlock(size, alignment);
  }

  template<typename T, typename... Args>
  NODISCARD T*
  create(Args&&... args)
  {
    void* memory = allocate(sizeof(T), alignof(T));
    T* object = new (memory) T(std::forward<Args>(args)...);
    if constexpr (!std::is_trivially_destructible_v<T>) {
      DestructorNode* node = new (allocate(sizeof(DestructorNode), alignof(DestructorNode)))
          DestructorNode{.destroy = [](void* target) { static_cast<T*>(target)->~T(); },
                         .object = object,
                         .next = m_destructors};
      m_destructors = node;
    }
    return object;
  }

  /**
   * Destroys the objects made with create() and makes every block free again.
   */
  void
  reset();

  /**
   * Bytes in every block, used or not.
   */
  NODISCARD SIZE_T
  getCapacity() const;

  NODISCARD FORCEINLINE SIZE_T
  getBlockCount() const
  {
    return m_blocks.size();
  }

 private:
  struct Block
  {
    uint8* data;
    SIZE_T size;
  };

  struct DestructorNode
  {
    void (*destroy)(void*);
    void* object;
    DestructorNode* next;
  };

  void*
  allocateFromNextBlock(SIZE_T size, SIZE_T alignment);

  Vector<Block> m_blocks;
  SIZE_T m_blockSize;
  SIZE_T m_currentBlock = 0;
  SIZE_T m_offset = 0;
  DestructorNode* m_destructors = nullptr;
};

} // namespace chEngineSDK
