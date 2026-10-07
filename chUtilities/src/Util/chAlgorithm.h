/************************************************************************/
/**
 * @file chAlgorithm.h
 * @author AccelMR
 * @date 2026/10/03
 * @brief Container algorithms used across the engine.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include <algorithm>

namespace chEngineSDK {

/**
 * Engine entry point for container algorithms, so code works on whole containers and
 * indices instead of iterator pairs. Every function is inline and forwards to the
 * standard library, so it costs the same as calling it directly. Include it only in the
 * .cpp files that need it, because <algorithm> is heavy.
 */
class Algorithm
{
 public:
  template<typename Container, typename T>
  NODISCARD static FORCEINLINE bool
  contains(const Container& container, const T& value)
  {
    return std::find(container.begin(), container.end(), value) != container.end();
  }

  /**
   * Erases the first element equal to value.
   *
   * @return false if no element was equal to value.
   */
  template<typename Container, typename T>
  static FORCEINLINE bool
  removeFirst(Container& container, const T& value)
  {
    const auto it = std::find(container.begin(), container.end(), value);
    if (it == container.end()) {
      return false;
    }
    container.erase(it);
    return true;
  }

  /**
   * Erases the first element for which matches returns true.
   *
   * @return false if no element matched.
   */
  template<typename Container, typename Predicate>
  static FORCEINLINE bool
  removeFirstIf(Container& container, Predicate matches)
  {
    const auto it = std::find_if(container.begin(), container.end(), matches);
    if (it == container.end()) {
      return false;
    }
    container.erase(it);
    return true;
  }

  /**
   * Erases every element equal to value.
   *
   * @return How many elements were erased.
   */
  template<typename Container, typename T>
  static FORCEINLINE SIZE_T
  removeAll(Container& container, const T& value)
  {
    const auto newEnd = std::remove(container.begin(), container.end(), value);
    const SIZE_T removed = static_cast<SIZE_T>(container.end() - newEnd);
    container.erase(newEnd, container.end());
    return removed;
  }

  template<typename Container>
  static FORCEINLINE void
  sort(Container& container)
  {
    std::sort(container.begin(), container.end());
  }

  /**
   * @param isBefore Returns true if its first argument goes before the second one.
   */
  template<typename Container, typename Compare>
  static FORCEINLINE void
  sort(Container& container, Compare isBefore)
  {
    std::sort(container.begin(), container.end(), isBefore);
  }

  /**
   * Index of the first element that is not less than value, or the size of the container
   * if there is none. The container must be sorted.
   */
  template<typename Container, typename T>
  NODISCARD static FORCEINLINE SIZE_T
  lowerBound(const Container& container, const T& value)
  {
    return static_cast<SIZE_T>(
        std::lower_bound(container.begin(), container.end(), value) - container.begin());
  }

  /**
   * Moves the element at firstIndex to the front, keeping the order of the rest. Puts a
   * ring buffer back in order when firstIndex is its oldest element.
   */
  template<typename Container>
  static FORCEINLINE void
  rotateToFront(Container& container, SIZE_T firstIndex)
  {
    CH_ASSERT(firstIndex <= container.size());
    std::rotate(container.begin(), container.begin() + firstIndex, container.end());
  }
};

} // namespace chEngineSDK
