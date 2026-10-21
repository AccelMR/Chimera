/************************************************************************/
/**
 * @file chTextureMips.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Builds the smaller mip levels of a texture on the CPU.
 */
/************************************************************************/

#include "chTextureMips.h"

#include "chFloat16.h"
#include "chMath.h"

#include <cstring>

namespace chEngineSDK {

namespace {
constexpr uint32 kChannels = 4;

float
srgbToLinear(float value)
{
  return value <= 0.04045f ? value / 12.92f : Math::pow((value + 0.055f) / 1.055f, 2.4f);
}

float
linearToSrgb(float value)
{
  return value <= 0.0031308f ? value * 12.92f
                             : 1.055f * Math::pow(value, 1.0f / 2.4f) - 0.055f;
}

uint8
toByte(float value)
{
  return static_cast<uint8>(Math::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

// Box filter: each texel of the next level averages the 2x2 texels above it (the last row
// or column repeats on odd sizes).
void
downsample(const Vector<float>& level,
           uint32 sourceWidth,
           uint32 sourceHeight,
           Vector<float>& nextLevel)
{
  const uint32 mipWidth = Math::max(sourceWidth / 2, 1u);
  const uint32 mipHeight = Math::max(sourceHeight / 2, 1u);
  nextLevel.resize(static_cast<SIZE_T>(mipWidth) * mipHeight * kChannels);

  for (uint32 y = 0; y < mipHeight; ++y) {
    const uint32 y0 = y * 2;
    const uint32 y1 = Math::min(y0 + 1, sourceHeight - 1);
    for (uint32 x = 0; x < mipWidth; ++x) {
      const uint32 x0 = x * 2;
      const uint32 x1 = Math::min(x0 + 1, sourceWidth - 1);
      const float* row0 = &level[static_cast<SIZE_T>(y0) * sourceWidth * kChannels];
      const float* row1 = &level[static_cast<SIZE_T>(y1) * sourceWidth * kChannels];
      float* destination = &nextLevel[(static_cast<SIZE_T>(y) * mipWidth + x) * kChannels];
      for (uint32 c = 0; c < kChannels; ++c) {
        destination[c] = (row0[x0 * kChannels + c] + row0[x1 * kChannels + c] +
                          row1[x0 * kChannels + c] + row1[x1 * kChannels + c]) *
                         0.25f;
      }
    }
  }
}

void
appendHalfs(const Vector<float>& level, Vector<uint8>& out)
{
  const SIZE_T start = out.size();
  out.resize(start + level.size() * sizeof(uint16));
  uint8* destination = out.data() + start;
  for (const float value : level) {
    const uint16 half = Float16::fromFloat(value);
    std::memcpy(destination, &half, sizeof(half));
    destination += sizeof(half);
  }
}
} // namespace

/*
 */
uint32
TextureMips::getMipCount(uint32 width, uint32 height) noexcept
{
  uint32 size = Math::max(width, height);
  uint32 count = 1;
  while (size > 1) {
    size /= 2;
    ++count;
  }
  return count;
}

/*
 */
uint32
TextureMips::appendChainRGBA8(Vector<uint8>& pixels, uint32 width, uint32 height, bool srgb)
{
  const uint32 mipCount = getMipCount(width, height);
  if (mipCount == 1) {
    return 1;
  }

  // Byte to float once per value, instead of a pow per channel of mip 0.
  Array<float, 256> byteToFloat;
  for (uint32 i = 0; i < 256; ++i) {
    const float value = static_cast<float>(i) / 255.0f;
    byteToFloat[i] = srgb ? srgbToLinear(value) : value;
  }

  // Each level is made from the one above kept in float, so rounding does not add up.
  Vector<float> level(static_cast<SIZE_T>(width) * height * kChannels);
  for (SIZE_T i = 0; i < level.size(); ++i) {
    const bool isAlpha = (i % kChannels) == 3;
    level[i] = isAlpha ? static_cast<float>(pixels[i]) / 255.0f : byteToFloat[pixels[i]];
  }

  SIZE_T totalSize = 0;
  for (uint32 mip = 0, w = width, h = height; mip < mipCount; ++mip) {
    totalSize += static_cast<SIZE_T>(w) * h * kChannels;
    w = Math::max(w / 2, 1u);
    h = Math::max(h / 2, 1u);
  }
  pixels.reserve(totalSize);

  Vector<float> nextLevel;
  uint32 sourceWidth = width;
  uint32 sourceHeight = height;
  for (uint32 mip = 1; mip < mipCount; ++mip) {
    const uint32 mipWidth = Math::max(sourceWidth / 2, 1u);
    const uint32 mipHeight = Math::max(sourceHeight / 2, 1u);
    downsample(level, sourceWidth, sourceHeight, nextLevel);

    for (SIZE_T i = 0; i < nextLevel.size(); ++i) {
      const bool isAlpha = (i % kChannels) == 3;
      pixels.push_back(toByte(srgb && !isAlpha ? linearToSrgb(nextLevel[i]) : nextLevel[i]));
    }

    level.swap(nextLevel);
    sourceWidth = mipWidth;
    sourceHeight = mipHeight;
  }
  return mipCount;
}

/*
 */
uint32
TextureMips::buildChainRGBA16F(Vector<float> pixels,
                               uint32 width,
                               uint32 height,
                               bool generateMips,
                               Vector<uint8>& outData)
{
  CH_ASSERT(pixels.size() == static_cast<SIZE_T>(width) * height * kChannels);
  const uint32 mipCount = generateMips ? getMipCount(width, height) : 1;

  SIZE_T totalTexels = 0;
  for (uint32 mip = 0, w = width, h = height; mip < mipCount; ++mip) {
    totalTexels += static_cast<SIZE_T>(w) * h;
    w = Math::max(w / 2, 1u);
    h = Math::max(h / 2, 1u);
  }
  outData.clear();
  outData.reserve(totalTexels * kChannels * sizeof(uint16));

  // Each level is made from the float level above, so the half rounding does not add up.
  appendHalfs(pixels, outData);
  Vector<float> nextLevel;
  uint32 sourceWidth = width;
  uint32 sourceHeight = height;
  for (uint32 mip = 1; mip < mipCount; ++mip) {
    downsample(pixels, sourceWidth, sourceHeight, nextLevel);
    appendHalfs(nextLevel, outData);
    pixels.swap(nextLevel);
    sourceWidth = Math::max(sourceWidth / 2, 1u);
    sourceHeight = Math::max(sourceHeight / 2, 1u);
  }
  return mipCount;
}

} // namespace chEngineSDK
