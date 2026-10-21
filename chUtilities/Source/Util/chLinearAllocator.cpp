/************************************************************************/
/**
 * @file chLinearAllocator.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Hands out memory by moving an offset, and frees all of it at once.
 */
/************************************************************************/

#include "chLinearAllocator.h"

#include <cstdlib>

#include "chMath.h"

namespace chEngineSDK {

/*
 */
LinearAllocator::LinearAllocator(SIZE_T blockSize)
  : m_blockSize(blockSize)
{}

/*
 */
LinearAllocator::~LinearAllocator()
{
  reset();
  for (const Block& block : m_blocks) {
    std::free(block.data);
  }
}

/*
 */
void
LinearAllocator::reset()
{
  for (DestructorNode* node = m_destructors; node != nullptr; node = node->next) {
    node->destroy(node->object);
  }
  m_destructors = nullptr;
  m_currentBlock = 0;
  m_offset = 0;
}

/*
 */
SIZE_T
LinearAllocator::getCapacity() const
{
  SIZE_T capacity = 0;
  for (const Block& block : m_blocks) {
    capacity += block.size;
  }
  return capacity;
}

/*
 */
void*
LinearAllocator::allocateFromNextBlock(SIZE_T size, SIZE_T alignment)
{
  CH_ASSERT(alignment != 0 && (alignment & (alignment - 1)) == 0);

  // Room for the worst alignment, so a block that passes this check always fits.
  const SIZE_T needed = size + alignment - 1;

  // Blocks kept from earlier frames come first; one too small for this request is left
  // unused until the next reset.
  SIZE_T next = m_blocks.empty() ? 0 : m_currentBlock + 1;
  while (next < m_blocks.size() && m_blocks[next].size < needed) {
    ++next;
  }

  if (next == m_blocks.size()) {
    const SIZE_T blockSize = Math::max(m_blockSize, needed);
    m_blocks.push_back({.data = static_cast<uint8*>(std::malloc(blockSize)),
                        .size = blockSize});
    CH_ASSERT(m_blocks.back().data != nullptr);
  }

  m_currentBlock = next;
  m_offset = 0;
  return allocate(size, alignment);
}

} // namespace chEngineSDK
