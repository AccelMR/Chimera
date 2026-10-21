/************************************************************************/
/**
 * @file chRandom.h
 * @author Victor Flores (lGaze)
 * @date 2022/09/10
 *
 * Random number generator class
 *
 * @bug No bug known.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes.
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK{
/*
 * Description:
 *     Random number generator class
 */
class CH_UTILITY_EXPORT Random
{
 public:

  /**
   * @brief Constructor using the initial given value as a starting seed
   *
   * By default seed value equals 0
   **/
  explicit FORCEINLINE
  Random(uint32 seed = 0);

  /**
   * @brief Default destructor
   **/
  ~Random() = default;

  /************************************************************************/
  /*
   * Methods.
   */
  /************************************************************************/

  /**
   * Sets the given value as a seed
   **/
  FORCEINLINE void
  setSeed(uint32 seed);

  /**
   * Gets a pseudo random number using Linear Congruential Generator
   **/
  FORCEINLINE uint32
  getPseudoRandom() const;

  /**
   * Gets a random number between 0 and 1
   **/
  NODISCARD FORCEINLINE float
  getRandom01() const {
    return static_cast<float>(getPseudoRandom()) / static_cast<float>(UINT32_MAX);
  }

  /************************************************************************/
  /*
   * Constants.
   */
  /************************************************************************/

 private:

  /**
   *  The "Multiplier" in the Linear Congruential Generator Formula
   */
  static const uint32 A;

  /**
   *  The "Increment" in the Linear Congruential Generator Formula
   */
  static const uint32 C;

  /************************************************************************/
  /*
   * Members.
   */
  /************************************************************************/

 private:

  /**
   *
   */
  mutable uint32 m_seed;
};

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
*/
FORCEINLINE
Random::Random(uint32 seed) : m_seed(seed) {}

/*
*/
FORCEINLINE uint32
Random::getPseudoRandom() const{
  uint32 newSeed = 0;

  // Linear Congruential Generator Formula
  newSeed = (A * m_seed) + C;

  m_seed = newSeed;
  return newSeed;
}

/*
*/
FORCEINLINE void
Random::setSeed(uint32 seed) {
  m_seed = seed;
}

}
