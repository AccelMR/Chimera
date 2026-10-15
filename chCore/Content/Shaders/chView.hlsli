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
  uint3 padding;
};
