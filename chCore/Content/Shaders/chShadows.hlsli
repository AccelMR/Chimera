// How much of a light reaches a point past what stands between them, read from the shadow
// maps the ShadowRenderer draws. They come in a raw buffer read by bindless index, one
// ShadowData after another, each followed by its cascades. A directional map is an atlas
// with one tile per cascade; each pixel reads the cascade its view depth falls in. A spot
// map is one tile of the spot atlas, seen through a perspective projection, with one
// cascade that covers every depth. A point map is one cube of a cube map array, read in the
// direction from the light; it has no cascades.
#pragma once

// Must match kNoShadow in chSceneViewBuffers.cpp.
static const uint kNoShadow = 0xFFFFFFFF;

// Must match ShadowMapType in chIRenderPath.h.
static const uint kShadowMapCascaded = 0;
static const uint kShadowMapSpot = 1;
static const uint kShadowMapPoint = 2;

// Must match GpuShadow in chSceneViewBuffers.h, up to its cascades.
struct ShadowData
{
  uint textureIndex;
  uint cascadeCount;
  // Texels around each pixel the test blends.
  uint filterRadius;
  // View depth where the shadow starts to fade.
  float fadeStart;
  // Size of one texel and of one tile in texture coordinates of the atlas. For a point map,
  // texelSize.x is one texel of a face per unit of distance from the light.
  float2 texelSize;
  float2 atlasScale;
  // View depth where each cascade ends, and where it starts to blend into the next one.
  float4 splitDepths;
  float4 blendStarts;
  // View depth where the shadow is gone.
  float fadeEnd;
  // kShadowMapCascaded, kShadowMapSpot or kShadowMapPoint.
  uint type;
  // How much of the whole shadow has faded; spot and point shadows fade with their light's
  // distance.
  float lightFade;
  float padding0;
  // Point maps: where the light is and the cube of the array.
  float3 lightPosition;
  uint cubeIndex;
  // Point maps: a face stores cubeDepthScale + cubeDepthOffset / d for a point at distance d
  // along its axis.
  float cubeDepthScale;
  float cubeDepthOffset;
  // Point maps: how far the receiver moves along its normal, per unit of distance.
  float cubeNormalOffset;
  float padding1;
};

// Must match GpuShadowCascade in chSceneViewBuffers.h. The matrix is kept as its rows, so it
// reads the same whatever matrix layout the raw load would pick.
struct ShadowCascadeData
{
  float4 viewProjectionRow0;
  float4 viewProjectionRow1;
  float4 viewProjectionRow2;
  float4 viewProjectionRow3;
  // Top left corner of the cascade's tile in the atlas.
  float2 atlasOffset;
  // Moves the receiver along its normal, so a surface does not shadow itself; per unit of
  // distance from the light in a perspective map.
  float normalOffset;
  float padding;
};

static const uint kShadowDataSize = 432;
static const uint kShadowCascadesOffset = 112;
static const uint kShadowCascadeDataSize = 80;

// Which cascade a view depth reads (cascadeCount past the last one) and how far it has
// blended into the next one.
uint
selectShadowCascade(ShadowData shadow, float viewDepth, out float blend)
{
  blend = 0.0f;
  for (uint i = 0; i < shadow.cascadeCount; ++i) {
    if (viewDepth <= shadow.splitDepths[i]) {
      const float blendStart = shadow.blendStarts[i];
      if (viewDepth > blendStart && i + 1 < shadow.cascadeCount) {
        blend = saturate((viewDepth - blendStart) /
                         max(shadow.splitDepths[i] - blendStart, 1e-4f));
      }
      return i;
    }
  }
  return shadow.cascadeCount;
}

// 1 where the light arrives in one cascade, 0 where something blocks it.
float
sampleShadowCascade(ShadowData shadow,
                    ByteAddressBuffer shadows,
                    uint shadowIndex,
                    uint cascadeIndex,
                    SamplerComparisonState shadowSampler,
                    float3 worldPosition,
                    float3 normal)
{
  const ShadowCascadeData cascade = shadows.Load<ShadowCascadeData>(
      shadowIndex * kShadowDataSize + kShadowCascadesOffset +
      cascadeIndex * kShadowCascadeDataSize);

  // Row vector times the matrix, as mul(float4(position, 1), viewProjection). The normal
  // offset is linear, so it is added in clip space; a perspective w is the distance in
  // front of the light, which is what one texel grows with.
  const float4 receiverClip = worldPosition.x * cascade.viewProjectionRow0 +
                              worldPosition.y * cascade.viewProjectionRow1 +
                              worldPosition.z * cascade.viewProjectionRow2 +
                              cascade.viewProjectionRow3;
  float normalOffset = cascade.normalOffset;
  if (shadow.type == kShadowMapSpot) {
    normalOffset *= receiverClip.w;
  }
  const float4 clip = receiverClip + normalOffset * (normal.x * cascade.viewProjectionRow0 +
                                                     normal.y * cascade.viewProjectionRow1 +
                                                     normal.z * cascade.viewProjectionRow2);
  // Behind a spot light: it does not light the point, so it cannot shadow it either.
  if (clip.w <= 0.0f) {
    return 1.0f;
  }
  const float3 ndc = clip.xyz / clip.w;
  const float2 tileCoord = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
  if (any(tileCoord < 0.0f) || any(tileCoord > 1.0f) || ndc.z > 1.0f) {
    return 1.0f;
  }

  Texture2D<float> shadowMap = ResourceDescriptorHeap[shadow.textureIndex];
  // Casters in front of the shadow map's near plane were clamped to depth 0.
  const float depth = saturate(ndc.z);
  const float2 texCoord = cascade.atlasOffset + tileCoord * shadow.atlasScale;
  // Taps stay inside the tile, so the filter never reads a neighbor cascade.
  const float2 tileMin = cascade.atlasOffset + shadow.texelSize * 0.5f;
  const float2 tileMax = cascade.atlasOffset + shadow.atlasScale - shadow.texelSize * 0.5f;

  // Each tap is already a bilinear blend of four comparisons.
  const int radius = (int)shadow.filterRadius;
  float lit = 0.0f;
  for (int y = -radius; y <= radius; ++y) {
    for (int x = -radius; x <= radius; ++x) {
      const float2 tap = clamp(texCoord + float2(x, y) * shadow.texelSize, tileMin, tileMax);
      lit += shadowMap.SampleCmpLevelZero(shadowSampler, tap, depth);
    }
  }
  const float width = (float)(radius * 2 + 1);
  return lit / (width * width);
}

// 1 where the light arrives in a point map, 0 where something blocks it. The face is the
// one the direction from the light points at most, and the stored depth follows the
// distance along that face's axis.
float
samplePointShadow(ShadowData shadow,
                  SamplerComparisonState shadowSampler,
                  float3 worldPosition,
                  float3 normal)
{
  float3 toReceiver = worldPosition - shadow.lightPosition;
  float3 axisDistances = abs(toReceiver);
  float faceDistance = max(axisDistances.x, max(axisDistances.y, axisDistances.z));
  // One texel of a face grows with the distance, so the offset does too.
  toReceiver += normal * (shadow.cubeNormalOffset * faceDistance);
  axisDistances = abs(toReceiver);
  faceDistance = max(axisDistances.x, max(axisDistances.y, axisDistances.z));
  if (faceDistance <= 1e-5f) {
    return 1.0f;
  }
  const float depth = shadow.cubeDepthScale + shadow.cubeDepthOffset / faceDistance;
  // Past the light's range: it does not light the point, so it cannot shadow it either.
  if (depth > 1.0f) {
    return 1.0f;
  }

  // The filter moves across the face, along its two other axes.
  float3 axisU = float3(0.0f, 1.0f, 0.0f);
  float3 axisV = float3(0.0f, 0.0f, 1.0f);
  if (axisDistances.y >= axisDistances.x && axisDistances.y >= axisDistances.z) {
    axisU = float3(1.0f, 0.0f, 0.0f);
  }
  else if (axisDistances.z >= axisDistances.x) {
    axisU = float3(1.0f, 0.0f, 0.0f);
    axisV = float3(0.0f, 1.0f, 0.0f);
  }
  const float step = shadow.texelSize.x * faceDistance;

  TextureCubeArray<float> shadowMap = ResourceDescriptorHeap[shadow.textureIndex];
  // Casters in front of the near plane were clamped to depth 0.
  const float compareDepth = saturate(depth);
  const float cube = (float)shadow.cubeIndex;
  const int radius = (int)shadow.filterRadius;
  float lit = 0.0f;
  for (int y = -radius; y <= radius; ++y) {
    for (int x = -radius; x <= radius; ++x) {
      const float3 tap = toReceiver + (axisU * (float)x + axisV * (float)y) * step;
      lit += shadowMap.SampleCmpLevelZero(shadowSampler, float4(tap, cube), compareDepth);
    }
  }
  const float width = (float)(radius * 2 + 1);
  return lit / (width * width);
}

// 1 where the light arrives, 0 where something blocks it, in between at the edges.
float
evaluateShadow(uint shadowBufferIndex,
               uint samplerIndex,
               uint shadowIndex,
               float3 worldPosition,
               float3 normal,
               float viewDepth)
{
  ByteAddressBuffer shadows = ResourceDescriptorHeap[shadowBufferIndex];
  const ShadowData shadow = shadows.Load<ShadowData>(shadowIndex * kShadowDataSize);

  if (shadow.type == kShadowMapPoint) {
    SamplerComparisonState pointSampler = SamplerDescriptorHeap[samplerIndex];
    return lerp(samplePointShadow(shadow, pointSampler, worldPosition, normal), 1.0f,
                shadow.lightFade);
  }

  float blend;
  const uint cascadeIndex = selectShadowCascade(shadow, viewDepth, blend);
  if (cascadeIndex >= shadow.cascadeCount) {
    return 1.0f;
  }

  SamplerComparisonState shadowSampler = SamplerDescriptorHeap[samplerIndex];
  float lit = sampleShadowCascade(shadow, shadows, shadowIndex, cascadeIndex, shadowSampler,
                                  worldPosition, normal);
  if (blend > 0.0f) {
    const float next = sampleShadowCascade(shadow, shadows, shadowIndex, cascadeIndex + 1,
                                           shadowSampler, worldPosition, normal);
    lit = lerp(lit, next, blend);
  }

  float fade = shadow.lightFade;
  if (shadow.type == kShadowMapCascaded) {
    fade = saturate((viewDepth - shadow.fadeStart) /
                    max(shadow.fadeEnd - shadow.fadeStart, 1e-4f));
  }
  return lerp(lit, 1.0f, fade);
}

// Color of the cascade a view depth reads, blended like the shadow; w is 0 past the last
// cascade.
float4
getShadowCascadeTint(uint shadowBufferIndex, uint shadowIndex, float viewDepth)
{
  static const float3 kCascadeColors[4] = {float3(1.0f, 0.15f, 0.15f),
                                           float3(0.15f, 1.0f, 0.15f),
                                           float3(0.2f, 0.4f, 1.0f),
                                           float3(1.0f, 0.9f, 0.1f)};
  ByteAddressBuffer shadows = ResourceDescriptorHeap[shadowBufferIndex];
  const ShadowData shadow = shadows.Load<ShadowData>(shadowIndex * kShadowDataSize);

  float blend;
  const uint cascadeIndex = selectShadowCascade(shadow, viewDepth, blend);
  if (cascadeIndex >= shadow.cascadeCount) {
    return float4(1.0f, 1.0f, 1.0f, 0.0f);
  }
  const uint nextIndex = min(cascadeIndex + 1, 3u);
  return float4(lerp(kCascadeColors[cascadeIndex], kCascadeColors[nextIndex], blend), 1.0f);
}
