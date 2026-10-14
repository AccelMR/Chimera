// Reads back what gbuffer.hlsl wrote. The passes that read it run at the size of the
// GBuffer, so each pixel loads its own texel without a sampler.
#pragma once

#include "chBRDF.hlsli"

// Bindless indexes of the GBuffer textures.
struct GBufferTextures
{
  uint baseColorIndex;
  uint normalIndex;
  uint surfaceIndex;
  uint emissiveIndex;
  uint depthIndex;
};

struct GBufferSample
{
  SurfaceData surface;
  float occlusion;
  float3 emissive;
};

// 1 where nothing was drawn.
float
loadGBufferDepth(GBufferTextures textures, int3 texel)
{
  Texture2D<float> depthTexture = ResourceDescriptorHeap[textures.depthIndex];
  return depthTexture.Load(texel);
}

GBufferSample
loadGBuffer(GBufferTextures textures, int3 texel)
{
  Texture2D<float4> baseColorTexture = ResourceDescriptorHeap[textures.baseColorIndex];
  Texture2D<float4> normalTexture = ResourceDescriptorHeap[textures.normalIndex];
  Texture2D<float4> surfaceTexture = ResourceDescriptorHeap[textures.surfaceIndex];
  Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[textures.emissiveIndex];

  const float4 surfaceTexel = surfaceTexture.Load(texel);

  GBufferSample result;
  result.surface.baseColor = baseColorTexture.Load(texel).rgb;
  result.surface.metallic = surfaceTexel.r;
  result.surface.roughness = surfaceTexel.g;
  // Ten bits per axis leave the vector a little off unit length.
  result.surface.normal = normalize(normalTexture.Load(texel).xyz * 2.0f - 1.0f);
  result.occlusion = surfaceTexel.b;
  result.emissive = emissiveTexture.Load(texel).rgb;
  return result;
}
