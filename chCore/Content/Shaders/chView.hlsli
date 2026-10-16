// What every lighting shader knows about the camera and the lights of the frame. It comes in
// a constant buffer read by bindless index.
#pragma once

// Must match ViewData in chSceneViewBuffers.cpp.
struct ViewData
{
  row_major float4x4 view;
  row_major float4x4 projection;
  // Takes a clip space position (NDC xy, depth) back to world space.
  row_major float4x4 inverseViewProjection;
  float3 cameraPosition;
  uint lightCount;
  uint lightBufferIndex;
  float ambient;
  // Shadow maps of the frame (chShadows.hlsli) and the sampler that compares against them.
  uint shadowBufferIndex;
  uint shadowSamplerIndex;
  // Shadow whose cascades the ShadowCascades view mode tints, kNoShadow in other modes.
  uint debugShadowIndex;
  // kFeature bits of the parts of the lighting that are on.
  uint featureFlags;
  uint2 padding;
};

// Must match the flags in chSceneViewBuffers.cpp.
static const uint kFeatureNormalMaps = 1u << 0;
static const uint kFeatureOcclusionMaps = 1u << 1;
static const uint kFeatureEmissive = 1u << 2;

bool
isFeatureOn(ViewData view, uint feature)
{
  return (view.featureFlags & feature) != 0;
}
