// First mip of the environment cube of the image based lighting: each texel reads the sky
// light's equirectangular image in its direction, turned by the sky light's rotation, so
// the cube is in world space and lighting samples it with world directions.

#include "chIBL.hlsli"

// Must match EnvironmentPushConstants in chImageBasedLighting.cpp.
// Flat, because a struct inside push constants is laid out differently by each target.
struct PushConstants
{
  // Rows of the rotation from world directions to directions of the image.
  float4 worldToSkyRow0;
  float4 worldToSkyRow1;
  float4 worldToSkyRow2;
  uint skyIndex;
  uint samplerIndex;
  uint outputIndex;
  uint size;
  // Mip of the image whose texels are about as large as those of the cube.
  float skyMipLevel;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

[numthreads(8, 8, 1)]
void
CSMain(uint3 threadId : SV_DispatchThreadID)
{
  if (any(threadId.xy >= g_push.size)) {
    return;
  }

  const float3 worldDirection = cubeTexelDirection(threadId, g_push.size);
  const float3x3 worldToSky =
      float3x3(g_push.worldToSkyRow0.xyz, g_push.worldToSkyRow1.xyz, g_push.worldToSkyRow2.xyz);
  const float3 skyDirection = mul(worldDirection, worldToSky);

  Texture2D<float4> sky = ResourceDescriptorHeap[g_push.skyIndex];
  SamplerState skySampler = SamplerDescriptorHeap[g_push.samplerIndex];
  const float3 color =
      sky.SampleLevel(skySampler, directionToEquirect(skyDirection), g_push.skyMipLevel).rgb;

  RWTexture2DArray<float4> output = ResourceDescriptorHeap[g_push.outputIndex];
  output[threadId] = float4(color, 1.0f);
}
