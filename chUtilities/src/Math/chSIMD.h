/************************************************************************/
/**
 * @file chSIMD.h
 * @author AccelMR
 * @date 2026/10/03
 * @brief Minimal 4-float SIMD layer: SSE2 on x64, NEON on ARM64, plain floats elsewhere.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#if USING(CH_ARCHITECTURE_X86_64)
# define CH_SIMD_SSE IN_USE
#elif USING(CH_ARCHITECTURE_ARM64)
# define CH_SIMD_NEON IN_USE
#endif

#if !defined(CH_SIMD_SSE)
# define CH_SIMD_SSE NOT_IN_USE
#endif
#if !defined(CH_SIMD_NEON)
# define CH_SIMD_NEON NOT_IN_USE
#endif

#if USING(CH_SIMD_SSE)
# include <emmintrin.h>
#elif USING(CH_SIMD_NEON)
# include <arm_neon.h>
#endif

namespace chEngineSDK {

/**
 * Gives the math classes one set of 4-float vector operations for every architecture,
 * so their hot paths are written once. Every function is inline and maps to one or two
 * instructions. SSE2 is part of every x64 CPU and NEON of every ARM64 CPU, so no runtime
 * check is needed.
 */
class SIMD
{
 public:
#if USING(CH_SIMD_SSE)
  using Float4 = __m128;
#elif USING(CH_SIMD_NEON)
  using Float4 = float32x4_t;
#else
  struct Float4
  {
    float v[4];
  };
#endif

  /**
   * Loads 4 floats from a 16-byte aligned address.
   */
  NODISCARD static FORCEINLINE Float4
  loadAligned(const float* source) noexcept
  {
#if USING(CH_SIMD_SSE)
    return _mm_load_ps(source);
#elif USING(CH_SIMD_NEON)
    return vld1q_f32(source);
#else
    return {{source[0], source[1], source[2], source[3]}};
#endif
  }

  NODISCARD static FORCEINLINE Float4
  loadUnaligned(const float* source) noexcept
  {
#if USING(CH_SIMD_SSE)
    return _mm_loadu_ps(source);
#elif USING(CH_SIMD_NEON)
    return vld1q_f32(source);
#else
    return {{source[0], source[1], source[2], source[3]}};
#endif
  }

  /**
   * Stores 4 floats to a 16-byte aligned address.
   */
  static FORCEINLINE void
  storeAligned(float* destination, Float4 value) noexcept
  {
#if USING(CH_SIMD_SSE)
    _mm_store_ps(destination, value);
#elif USING(CH_SIMD_NEON)
    vst1q_f32(destination, value);
#else
    for (int32 i = 0; i < 4; ++i) {
      destination[i] = value.v[i];
    }
#endif
  }

  static FORCEINLINE void
  storeUnaligned(float* destination, Float4 value) noexcept
  {
#if USING(CH_SIMD_SSE)
    _mm_storeu_ps(destination, value);
#elif USING(CH_SIMD_NEON)
    vst1q_f32(destination, value);
#else
    for (int32 i = 0; i < 4; ++i) {
      destination[i] = value.v[i];
    }
#endif
  }

  /**
   * Returns value * scalar, element by element.
   */
  NODISCARD static FORCEINLINE Float4
  multiply(Float4 value, float scalar) noexcept
  {
#if USING(CH_SIMD_SSE)
    return _mm_mul_ps(value, _mm_set1_ps(scalar));
#elif USING(CH_SIMD_NEON)
    return vmulq_n_f32(value, scalar);
#else
    return {{value.v[0] * scalar, value.v[1] * scalar, value.v[2] * scalar,
             value.v[3] * scalar}};
#endif
  }

  /**
   * Returns accumulator + value * scalar, element by element.
   */
  NODISCARD static FORCEINLINE Float4
  multiplyAdd(Float4 accumulator, Float4 value, float scalar) noexcept
  {
#if USING(CH_SIMD_SSE)
    return _mm_add_ps(accumulator, _mm_mul_ps(value, _mm_set1_ps(scalar)));
#elif USING(CH_SIMD_NEON)
    return vfmaq_n_f32(accumulator, value, scalar);
#else
    return {{accumulator.v[0] + value.v[0] * scalar, accumulator.v[1] + value.v[1] * scalar,
             accumulator.v[2] + value.v[2] * scalar, accumulator.v[3] + value.v[3] * scalar}};
#endif
  }
};

} // namespace chEngineSDK
