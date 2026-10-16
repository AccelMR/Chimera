// One direction of the blur of the ambient occlusion: averages the texels along a row or a
// column that lie on the same surface as the center, so the noise goes away but the
// occlusion does not leak across depth edges.

#include "chBindless.hlsli"
#include "chScreenSpaceAO.hlsli"

// Must match BlurPushConstants in chScreenSpaceAO.cpp.
struct PushConstants
{
  uint inputIndex;
  uint outputIndex;
  uint depthIndex;
  uint viewIndex;
  uint width;
  uint height;
  uint depthWidth;
  uint depthHeight;
  uint resolutionScale;
  int directionX;
  int directionY;
  uint radius;
  float depthTolerance;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

// Raw depth of the pixel an occlusion texel was measured at.
float
loadDepth(Texture2D<float> depthTexture, int2 texel)
{
  const int2 maxTexel = int2(g_push.depthWidth, g_push.depthHeight) - 1;
  return depthTexture.Load(int3(min(texel * int(g_push.resolutionScale), maxTexel), 0));
}

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
  const float centerRawDepth = loadDepth(depthTexture, texel);
  if (centerRawDepth >= 1.0f) {
    output[texel] = 1.0f;
    return;
  }

  const ViewData view = loadConstants<ViewData>(g_push.viewIndex);
  const int2 direction = int2(g_push.directionX, g_push.directionY);
  const int2 maxTexel = int2(g_push.width, g_push.height) - 1;
  const float centerDepth = linearizeDepth(centerRawDepth, view);

  // The depth of a surface that is not facing the camera changes along the row, so the
  // neighbors are compared with the slope of the surface, taken from the side that changes
  // the least: the other side may be across an edge.
  const float nextDepth =
      linearizeDepth(loadDepth(depthTexture, clamp(texel + direction, 0, maxTexel)), view);
  const float previousDepth =
      linearizeDepth(loadDepth(depthTexture, clamp(texel - direction, 0, maxTexel)), view);
  const float slope = abs(nextDepth - centerDepth) < abs(centerDepth - previousDepth)
                          ? nextDepth - centerDepth
                          : centerDepth - previousDepth;

  float sum = input.Load(int3(texel, 0));
  float weightSum = 1.0f;
  const int radius = int(g_push.radius);
  for (int i = -radius; i <= radius; ++i) {
    if (i == 0) {
      continue;
    }
    const int2 neighbor = texel + direction * i;
    if (any(neighbor < 0) || any(neighbor > maxTexel)) {
      continue;
    }
    const float neighborDepth = linearizeDepth(loadDepth(depthTexture, neighbor), view);
    const float weight = getDepthWeight(neighborDepth, centerDepth + slope * i, centerDepth,
                                        g_push.depthTolerance);
    sum += input.Load(int3(neighbor, 0)) * weight;
    weightSum += weight;
  }
  output[texel] = sum / weightSum;
}
