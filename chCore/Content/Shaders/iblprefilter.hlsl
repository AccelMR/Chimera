// One mip of the prefiltered cube of the image based lighting: the environment blurred by
// the GGX lobe of the mip's roughness, the first half of the split sum (the lookup table of
// brdflut.hlsl is the other). The lobe is taken around the reflection with the view along
// the normal, which loses the stretched highlights seen at grazing angles but makes the
// result depend on the direction only.

#include "chIBL.hlsli"

// Must match PrefilterPushConstants in chImageBasedLighting.cpp.
struct PushConstants
{
  uint environmentIndex;
  uint samplerIndex;
  uint outputIndex;
  uint size;
  uint sampleCount;
  uint environmentSize;
  float environmentMaxMip;
  float roughness;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

[numthreads(8, 8, 1)]
void
CSMain(uint3 threadId : SV_DispatchThreadID)
{
  if (any(threadId.xy >= g_push.size)) {
    return;
  }

  TextureCube<float4> environment = ResourceDescriptorHeap[g_push.environmentIndex];
  SamplerState environmentSampler = SamplerDescriptorHeap[g_push.samplerIndex];
  const float3 normal = cubeTexelDirection(threadId, g_push.size);
  RWTexture2DArray<float4> output = ResourceDescriptorHeap[g_push.outputIndex];

  // A mirror reflects one direction: the environment's mip whose texels match this one.
  if (g_push.roughness <= 0.0f) {
    const float mip = log2((float)g_push.environmentSize / g_push.size);
    output[threadId] = environment.SampleLevel(environmentSampler, normal, mip);
    return;
  }

  const float alpha = g_push.roughness * g_push.roughness;
  float3 sum = float3(0.0f, 0.0f, 0.0f);
  float weight = 0.0f;
  for (uint i = 0; i < g_push.sampleCount; ++i) {
    const float3 halfVector =
        tangentToWorld(importanceSampleGGX(hammersley(i, g_push.sampleCount), alpha), normal);
    const float normalDotHalf = saturate(dot(normal, halfVector));
    const float3 lightDirection = 2.0f * normalDotHalf * halfVector - normal;
    const float normalDotLight = dot(normal, lightDirection);
    if (normalDotLight <= 0.0f) {
      continue;
    }

    // With the view along the normal, NdotH equals VdotH, so the pdf of the light
    // direction D * NdotH / (4 * VdotH) is D / 4.
    const float pdf = distributionGGX(normalDotHalf, alpha) * 0.25f;
    const float mip = min(sampleMipLevel(pdf, g_push.sampleCount, g_push.environmentSize),
                          g_push.environmentMaxMip);
    sum += environment.SampleLevel(environmentSampler, lightDirection, mip).rgb *
           normalDotLight;
    weight += normalDotLight;
  }

  output[threadId] = float4(sum / max(weight, 1e-4f), 1.0f);
}
