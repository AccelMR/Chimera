// Irradiance cube of the image based lighting: for each normal, the light of the
// environment over the hemisphere around it weighted by the cosine and divided by pi, so a
// Lambert surface of color c reflects c times the texel.

#include "chIBL.hlsli"

// Must match IrradiancePushConstants in chImageBasedLighting.cpp.
struct PushConstants
{
  uint environmentIndex;
  uint samplerIndex;
  uint outputIndex;
  uint size;
  uint sampleCount;
  uint environmentSize;
  float environmentMaxMip;
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

  // With samples spread by the cosine, the average of what they see is the result.
  float3 sum = float3(0.0f, 0.0f, 0.0f);
  for (uint i = 0; i < g_push.sampleCount; ++i) {
    const float3 local = cosineSampleHemisphere(hammersley(i, g_push.sampleCount));
    const float pdf = local.z / kPi;
    const float mip = min(sampleMipLevel(pdf, g_push.sampleCount, g_push.environmentSize),
                          g_push.environmentMaxMip);
    sum += environment.SampleLevel(environmentSampler, tangentToWorld(local, normal), mip).rgb;
  }

  RWTexture2DArray<float4> output = ResourceDescriptorHeap[g_push.outputIndex];
  output[threadId] = float4(sum / g_push.sampleCount, 1.0f);
}
