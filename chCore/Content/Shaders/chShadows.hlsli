// How much of a light reaches a point past what stands between them, read from the shadow
// maps the ShadowRenderer draws. They come in a raw buffer read by bindless index, one
// ShadowData after another.
#pragma once

// Must match kNoShadow in chSceneViewBuffers.cpp.
static const uint kNoShadow = 0xFFFFFFFF;

// Must match GpuShadow in chSceneViewBuffers.h. The matrix is kept as its rows, so it reads
// the same whatever matrix layout the raw load would pick.
struct ShadowData
{
  float4 viewProjectionRow0;
  float4 viewProjectionRow1;
  float4 viewProjectionRow2;
  float4 viewProjectionRow3;
  uint textureIndex;
  // Size of one texel in texture coordinates.
  float texelSize;
  // Moves the receiver along its normal, so a surface does not shadow itself.
  float normalOffset;
  // Distance from the camera where the shadow has faded out.
  float fadeDistance;
};

static const uint kShadowDataSize = 80;

// The last part of the shadow distance fades out, so the shadow has no hard edge.
static const float kShadowFadeFraction = 0.2f;

// 1 where the light arrives, 0 where something blocks it, in between at the edges.
float
evaluateShadow(uint shadowBufferIndex,
               uint samplerIndex,
               uint shadowIndex,
               float3 worldPosition,
               float3 normal,
               float3 cameraPosition)
{
  ByteAddressBuffer shadows = ResourceDescriptorHeap[shadowBufferIndex];
  const ShadowData shadow = shadows.Load<ShadowData>(shadowIndex * kShadowDataSize);

  const float3 position = worldPosition + normal * shadow.normalOffset;
  // Row vector times the matrix, as mul(float4(position, 1), viewProjection).
  const float4 clip = position.x * shadow.viewProjectionRow0 +
                      position.y * shadow.viewProjectionRow1 +
                      position.z * shadow.viewProjectionRow2 + shadow.viewProjectionRow3;
  const float3 ndc = clip.xyz / clip.w;
  const float2 texCoord = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
  if (any(texCoord < 0.0f) || any(texCoord > 1.0f) || ndc.z > 1.0f) {
    return 1.0f;
  }

  Texture2D<float> shadowMap = ResourceDescriptorHeap[shadow.textureIndex];
  SamplerComparisonState shadowSampler = SamplerDescriptorHeap[samplerIndex];
  // Casters in front of the shadow map's near plane were clamped to depth 0.
  const float depth = saturate(ndc.z);

  // 3x3 taps, each one already a bilinear blend of four comparisons.
  float lit = 0.0f;
  [unroll]
  for (int y = -1; y <= 1; ++y) {
    [unroll]
    for (int x = -1; x <= 1; ++x) {
      const float2 offset = float2(x, y) * shadow.texelSize;
      lit += shadowMap.SampleCmpLevelZero(shadowSampler, texCoord + offset, depth);
    }
  }
  lit /= 9.0f;

  const float fadeStart = shadow.fadeDistance * (1.0f - kShadowFadeFraction);
  const float fade = saturate((distance(worldPosition, cameraPosition) - fadeStart) /
                              max(shadow.fadeDistance - fadeStart, 1e-4f));
  return lerp(lit, 1.0f, fade);
}
