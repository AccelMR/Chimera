// Color space helpers. Lighting works in linear values; the back buffer holds sRGB values
// in a UNORM format, so whatever writes to it encodes them.
#pragma once

float3
linearToSrgb(float3 color)
{
  const float3 low = color * 12.92f;
  const float3 high = 1.055f * pow(color, 1.0f / 2.4f) - 0.055f;
  return select(color <= 0.0031308f, low, high);
}

// ACES filmic curve fitted by Stephen Hill: takes linear scene light (any brightness) to
// [0, 1] with a soft shoulder, so bright lights do not clip and colors desaturate toward
// white as film does.
float3
tonemapACES(float3 color)
{
  // sRGB primaries => ACES input, with the reference rendering transform's exposure.
  const float3x3 inputMatrix = {0.59719f, 0.35458f, 0.04823f,
                                0.07600f, 0.90834f, 0.01566f,
                                0.02840f, 0.13383f, 0.83777f};
  // Output transform => sRGB primaries.
  const float3x3 outputMatrix = {1.60475f, -0.53108f, -0.07367f,
                                 -0.10208f, 1.10813f, -0.00605f,
                                 -0.00327f, -0.07276f, 1.07602f};

  color = mul(inputMatrix, color);
  const float3 a = color * (color + 0.0245786f) - 0.000090537f;
  const float3 b = color * (0.983729f * color + 0.4329510f) + 0.238081f;
  color = mul(outputMatrix, a / b);
  return saturate(color);
}
