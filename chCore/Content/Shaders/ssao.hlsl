// Ground truth ambient occlusion (Jimenez et al. 2016, following Intel's XeGTAO): in a few
// directions around each pixel, walks the depth buffer on both sides to find the highest
// horizon nearby surfaces make, then integrates the cosine weighted visibility between the
// two horizons. Writes 1 where nothing occludes.

#include "chBindless.hlsli"
#include "chBRDF.hlsli"
#include "chScreenSpaceAO.hlsli"

// Must match OcclusionPushConstants in chScreenSpaceAO.cpp.
struct PushConstants
{
  uint depthIndex;
  uint normalIndex;
  uint outputIndex;
  uint viewIndex;
  uint outputWidth;
  uint outputHeight;
  uint depthWidth;
  uint depthHeight;
  uint resolutionScale;
  uint sliceCount;
  uint stepCount;
  float radius;
  float falloffRange;
  float power;
  float maxPixelRadius;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

// Samples closer than this, in pixels, would read the pixel itself.
static const float kMinSampleDistance = 1.3f;

// Order of a 4x4 Bayer matrix: neighbors get values far apart, so a 4 texel blur averages
// every rotation of the directions.
static const uint kBayer4x4[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

float
loadDepth(Texture2D<float> depthTexture, int2 texel)
{
  const int2 maxTexel = int2(g_push.depthWidth, g_push.depthHeight) - 1;
  return depthTexture.Load(int3(clamp(texel, 0, maxTexel), 0));
}

float3
loadViewPosition(Texture2D<float> depthTexture, int2 texel, float2 inverseSize, ViewData view)
{
  const int2 maxTexel = int2(g_push.depthWidth, g_push.depthHeight) - 1;
  const int2 clamped = clamp(texel, 0, maxTexel);
  const float depth = depthTexture.Load(int3(clamped, 0));
  return viewPositionFromDepth((clamped + 0.5f) * inverseSize, depth, view);
}

float
computeVisibility(int2 texel, Texture2D<float> depthTexture, float depth, ViewData view)
{
  const float2 inverseSize = 1.0f / float2(g_push.depthWidth, g_push.depthHeight);
  const float3 position = viewPositionFromDepth((texel + 0.5f) * inverseSize, depth, view);

  Texture2D<float4> normalTexture = ResourceDescriptorHeap[g_push.normalIndex];
  const float3 worldNormal = normalTexture.Load(int3(texel, 0)).xyz * 2.0f - 1.0f;
  const float3 normal = normalize(mul(float4(worldNormal, 0.0f), view.view).xyz);
  const float3 viewVector = normalize(-position);

  const float pixelRadius = min(
      g_push.radius / getViewPixelSize(position.z, g_push.depthWidth, view),
      g_push.maxPixelRadius);
  if (pixelRadius < kMinSampleDistance) {
    return 1.0f;
  }

  // Occluders fade out over the last part of the radius.
  const float falloffDistance = g_push.falloffRange * g_push.radius;
  const float falloffMultiply = -1.0f / falloffDistance;
  const float falloffAdd = (g_push.radius - falloffDistance) / falloffDistance + 1.0f;

  // R2 sequence of the Bayer order: a different rotation and step offset per pixel of a
  // 4x4 block, spread evenly.
  const uint2 block = uint2(texel) & 3u;
  const float noiseIndex = (float)kBayer4x4[block.y * 4 + block.x];
  const float noiseSlice = frac(0.5f + noiseIndex * 0.7548776662f);
  const float noiseStep = frac(0.5f + noiseIndex * 0.5698402910f);

  const float minStep = kMinSampleDistance / pixelRadius;
  float visibility = 0.0f;
  for (uint slice = 0; slice < g_push.sliceCount; ++slice) {
    const float angle = (slice + noiseSlice) / g_push.sliceCount * kPi;
    float sinAngle;
    float cosAngle;
    sincos(angle, sinAngle, cosAngle);
    // Pixels go down the image while view space Y goes up.
    const float2 screenDirection = float2(cosAngle, -sinAngle) * pixelRadius;
    const float3 direction = float3(cosAngle, sinAngle, 0.0f);

    // The slice is the plane through the view vector and the direction; the normal is
    // projected into it, and n is its angle from the view vector.
    const float3 orthoDirection = direction - dot(direction, viewVector) * viewVector;
    const float3 axis = normalize(cross(orthoDirection, viewVector));
    const float3 projectedNormal = normal - axis * dot(normal, axis);
    const float projectedNormalLength = max(length(projectedNormal), 1e-5f);
    const float signNormal = sign(dot(orthoDirection, projectedNormal));
    const float cosNormal = saturate(dot(projectedNormal, viewVector) / projectedNormalLength);
    const float normalAngle = signNormal * acos(cosNormal);

    // The lowest horizons on each side are the plane of the surface itself.
    const float lowHorizonCos0 = cos(normalAngle + kPi * 0.5f);
    const float lowHorizonCos1 = cos(normalAngle - kPi * 0.5f);
    float horizonCos0 = lowHorizonCos0;
    float horizonCos1 = lowHorizonCos1;

    for (uint stepIndex = 0; stepIndex < g_push.stepCount; ++stepIndex) {
      const float stepNoise =
          frac(noiseStep + (slice + stepIndex * g_push.stepCount) * 0.6180339887f);
      // Squared, so more samples land close to the pixel, where occluders matter most.
      float stepDistance = (stepIndex + stepNoise) / g_push.stepCount;
      stepDistance = stepDistance * stepDistance + minStep;
      const int2 offset = int2(round(screenDirection * stepDistance));

      const float3 delta0 =
          loadViewPosition(depthTexture, texel + offset, inverseSize, view) - position;
      const float3 delta1 =
          loadViewPosition(depthTexture, texel - offset, inverseSize, view) - position;
      const float length0 = length(delta0);
      const float length1 = length(delta1);
      const float weight0 = saturate(length0 * falloffMultiply + falloffAdd);
      const float weight1 = saturate(length1 * falloffMultiply + falloffAdd);
      const float sampleCos0 =
          lerp(lowHorizonCos0, dot(delta0 / length0, viewVector), weight0);
      const float sampleCos1 =
          lerp(lowHorizonCos1, dot(delta1 / length1, viewVector), weight1);
      horizonCos0 = max(horizonCos0, sampleCos0);
      horizonCos1 = max(horizonCos1, sampleCos1);
    }

    // Side 0 is along the direction, which has positive angles from the view vector.
    float horizon0 = -acos(horizonCos1);
    float horizon1 = acos(horizonCos0);
    horizon0 = normalAngle + clamp(horizon0 - normalAngle, -kPi * 0.5f, kPi * 0.5f);
    horizon1 = normalAngle + clamp(horizon1 - normalAngle, -kPi * 0.5f, kPi * 0.5f);
    const float sinNormal = sin(normalAngle);
    const float arc0 =
        (cosNormal + 2.0f * horizon0 * sinNormal - cos(2.0f * horizon0 - normalAngle)) * 0.25f;
    const float arc1 =
        (cosNormal + 2.0f * horizon1 * sinNormal - cos(2.0f * horizon1 - normalAngle)) * 0.25f;
    visibility += projectedNormalLength * (arc0 + arc1);
  }

  visibility /= g_push.sliceCount;
  return saturate(pow(saturate(visibility), g_push.power));
}

[numthreads(8, 8, 1)]
void
CSMain(uint3 threadId : SV_DispatchThreadID)
{
  if (threadId.x >= g_push.outputWidth || threadId.y >= g_push.outputHeight) {
    return;
  }

  RWTexture2D<float> output = ResourceDescriptorHeap[g_push.outputIndex];
  Texture2D<float> depthTexture = ResourceDescriptorHeap[g_push.depthIndex];
  // At half resolution each texel measures the top left pixel of its 2x2 block.
  const int2 texel = int2(threadId.xy * g_push.resolutionScale);
  const float depth = loadDepth(depthTexture, texel);
  if (depth >= 1.0f) {
    output[threadId.xy] = 1.0f;
    return;
  }

  const ViewData view = loadConstants<ViewData>(g_push.viewIndex);
  output[threadId.xy] = computeVisibility(texel, depthTexture, depth, view);
}
