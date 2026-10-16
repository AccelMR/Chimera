// Lookup table of the split sum approximation: for each NdotV (x) and roughness (y), the
// scale (r) and bias (g) that turn the reflectance at normal incidence into the specular
// reflectance integrated over the GGX lobe, with the visibility term of chBRDF.hlsli.

#include "chIBL.hlsli"

// Must match BrdfLutPushConstants in chImageBasedLighting.cpp.
struct PushConstants
{
  uint outputIndex;
  uint size;
  uint sampleCount;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

[numthreads(8, 8, 1)]
void
CSMain(uint3 threadId : SV_DispatchThreadID)
{
  if (any(threadId.xy >= g_push.size)) {
    return;
  }

  // Texel centers, so the table never holds NdotV = 0, where the visibility term divides
  // by zero.
  const float normalDotView = (threadId.x + 0.5f) / g_push.size;
  const float roughness = (threadId.y + 0.5f) / g_push.size;
  const float alpha = roughness * roughness;

  // The normal is +Z; only the angle to the view matters.
  const float3 viewDirection =
      float3(sqrt(1.0f - normalDotView * normalDotView), 0.0f, normalDotView);

  float scale = 0.0f;
  float bias = 0.0f;
  for (uint i = 0; i < g_push.sampleCount; ++i) {
    const float3 halfVector = importanceSampleGGX(hammersley(i, g_push.sampleCount), alpha);
    const float viewDotHalf = saturate(dot(viewDirection, halfVector));
    const float3 lightDirection = 2.0f * viewDotHalf * halfVector - viewDirection;
    const float normalDotLight = saturate(lightDirection.z);
    if (normalDotLight <= 0.0f) {
      continue;
    }

    // The BRDF times the cosine over the probability of the sample; the distribution
    // cancels out.
    const float normalDotHalf = saturate(halfVector.z);
    const float weight = visibilitySmithGGXCorrelated(normalDotView, normalDotLight, alpha) *
                         4.0f * normalDotLight * viewDotHalf / max(normalDotHalf, 1e-5f);
    const float fresnel = pow(1.0f - viewDotHalf, 5.0f);
    scale += (1.0f - fresnel) * weight;
    bias += fresnel * weight;
  }

  RWTexture2D<float2> output = ResourceDescriptorHeap[g_push.outputIndex];
  output[threadId.xy] = float2(scale, bias) / g_push.sampleCount;
}
