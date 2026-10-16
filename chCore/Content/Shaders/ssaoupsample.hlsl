// Scales the half resolution ambient occlusion up to the full image: the four nearest
// texels are blended like a bilinear filter, but only those measured on the same surface as
// the pixel count, so edges stay sharp.

#include "chBindless.hlsli"
#include "chScreenSpaceAO.hlsli"

// Must match UpsamplePushConstants in chScreenSpaceAO.cpp.
struct PushConstants
{
  uint inputIndex;
  uint outputIndex;
  uint depthIndex;
  uint viewIndex;
  uint inputWidth;
  uint inputHeight;
  uint width;
  uint height;
  float depthTolerance;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

[numthreads(8, 8, 1)]
void
CSMain(uint3 threadId : SV_DispatchThreadID)
{
  if (threadId.x >= g_push.width || threadId.y >= g_push.height) {
    return;
  }

  Texture2D<float> input = ResourceDescriptorHeap[g_push.inputIndex];
  Texture2D<float> depthTexture = ResourceDescriptorHeap[g_push.depthIndex];
  RWTexture2D<float> output = ResourceDescriptorHeap[g_push.outputIndex];

  const int2 texel = int2(threadId.xy);
  const float rawDepth = depthTexture.Load(int3(texel, 0));
  if (rawDepth >= 1.0f) {
    output[texel] = 1.0f;
    return;
  }

  const ViewData view = loadConstants<ViewData>(g_push.viewIndex);
  const float centerDepth = linearizeDepth(rawDepth, view);
  const int2 maxInput = int2(g_push.inputWidth, g_push.inputHeight) - 1;
  const int2 maxTexel = int2(g_push.width, g_push.height) - 1;

  // Each input texel was measured at the top left pixel of its 2x2 block.
  const float2 inputPosition = texel * 0.5f;
  const int2 base = int2(floor(inputPosition));
  const float2 fraction = inputPosition - base;

  float sum = 0.0f;
  float weightSum = 0.0f;
  float nearestValue = 1.0f;
  float nearestDifference = 1e30f;
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      const int2 inputTexel = min(base + int2(x, y), maxInput);
      const float value = input.Load(int3(inputTexel, 0));
      const float sampleDepth = linearizeDepth(
          depthTexture.Load(int3(min(inputTexel * 2, maxTexel), 0)), view);
      const float2 bilinear = lerp(1.0f - fraction, fraction, float2(x, y));
      const float weight = bilinear.x * bilinear.y *
                           getDepthWeight(sampleDepth, centerDepth, centerDepth,
                                          g_push.depthTolerance);
      sum += value * weight;
      weightSum += weight;

      const float difference = abs(sampleDepth - centerDepth);
      if (difference < nearestDifference) {
        nearestDifference = difference;
        nearestValue = value;
      }
    }
  }
  // No texel on this surface (a thin object at half resolution): the closest in depth.
  output[texel] = weightSum > 1e-4f ? sum / weightSum : nearestValue;
}
