// The scene lights a lighting shader loops over. They come in a raw buffer read by bindless
// index, one LightData after another.
#pragma once

#include "chBRDF.hlsli"
#include "chView.hlsli"
#include "chShadows.hlsli"

// Must match LightType in chLightItem.h.
static const uint kLightDirectional = 0;
static const uint kLightPoint = 1;
static const uint kLightSpot = 2;

// Must match GpuLight in chSceneViewBuffers.h.
struct LightData
{
  float3 position;
  uint type;
  // Where the light travels.
  float3 direction;
  float range;
  // Already multiplied by the intensity.
  float3 color;
  // The spot falloff is saturate(cos * spotScale + spotOffset), from the cone cosines.
  float spotScale;
  float spotOffset;
  // Index into the shadow buffer of the view, kNoShadow without a shadow map.
  uint shadowIndex;
  float2 padding;
};

static const uint kLightDataSize = 64;

// Inverse square of the distance, brought down to zero at the range so the light can be
// skipped past it without a visible edge (the falloff of Unreal and Frostbite).
float
getDistanceAttenuation(float distanceSquared, float range)
{
  const float rangeSquared = range * range;
  const float ratio = distanceSquared / max(rangeSquared, 1e-8f);
  const float window = saturate(1.0f - ratio * ratio);
  return window * window / max(distanceSquared, 1e-4f);
}

// Light sent towards the eye by every light of the view.
float3
evaluateLights(SurfaceData surface, float3 worldPosition, float3 viewDirection, ViewData view)
{
  ByteAddressBuffer lights = ResourceDescriptorHeap[view.lightBufferIndex];
  const float viewDepth = mul(float4(worldPosition, 1.0f), view.view).z;
  float3 result = float3(0.0f, 0.0f, 0.0f);
  for (uint i = 0; i < view.lightCount; ++i) {
    const LightData light = lights.Load<LightData>(i * kLightDataSize);

    float3 lightDirection = -light.direction;
    float attenuation = 1.0f;
    if (light.type != kLightDirectional) {
      const float3 toLight = light.position - worldPosition;
      const float distanceSquared = dot(toLight, toLight);
      lightDirection = toLight * rsqrt(max(distanceSquared, 1e-8f));
      attenuation = getDistanceAttenuation(distanceSquared, light.range);
      if (light.type == kLightSpot) {
        const float cosine = dot(-lightDirection, light.direction);
        const float cone = saturate(cosine * light.spotScale + light.spotOffset);
        attenuation *= cone * cone;
      }
    }

    if (attenuation > 0.0f && light.shadowIndex != kNoShadow) {
      attenuation *= evaluateShadow(view.shadowBufferIndex, view.shadowSamplerIndex,
                                    light.shadowIndex, worldPosition, surface.normal,
                                    viewDepth);
    }

    if (attenuation > 0.0f) {
      result += evaluateBRDF(surface, viewDirection, lightDirection) * light.color *
                attenuation;
    }
  }
  return result;
}

// Light from the environment: the sky light's diffuse and reflections (image based
// lighting), or without one the constant ambient, which is only diffuse because there is
// nothing for it to reflect. viewDirection points away from the surface.
float3
evaluateAmbient(SurfaceData surface, float3 viewDirection, float occlusion, ViewData view)
{
  const float3 diffuseColor = surface.baseColor * (1.0f - surface.metallic);
  if (view.irradianceIndex == kNoEnvironment) {
    return diffuseColor * view.ambient * occlusion;
  }

  TextureCube<float4> irradiance = ResourceDescriptorHeap[view.irradianceIndex];
  TextureCube<float4> prefiltered = ResourceDescriptorHeap[view.prefilteredIndex];
  Texture2D<float2> brdfLut = ResourceDescriptorHeap[view.brdfLutIndex];
  SamplerState environmentSampler = SamplerDescriptorHeap[view.environmentSamplerIndex];

  // Split sum: the prefiltered reflection times the integrated BRDF, which turns the
  // reflectance at normal incidence into the one over the whole lobe.
  const float normalDotView = max(dot(surface.normal, viewDirection), 1e-4f);
  const float roughness = max(surface.roughness, kMinRoughness);
  const float3 reflectance0 =
      lerp(kDielectricReflectance.xxx, surface.baseColor, surface.metallic);
  const float2 scaleBias =
      brdfLut.SampleLevel(environmentSampler, float2(normalDotView, roughness), 0.0f);
  const float3 specularColor = reflectance0 * scaleBias.x + scaleBias.y;
  const float3 reflection = reflect(-viewDirection, surface.normal);
  const float3 specular =
      prefiltered.SampleLevel(environmentSampler, reflection,
                              roughness * view.prefilteredMaxMip).rgb * specularColor;

  // What the surface reflects as specular is not diffused.
  const float3 diffuse = irradiance.SampleLevel(environmentSampler, surface.normal, 0.0f).rgb *
                         diffuseColor * (1.0f - specularColor);

  return (diffuse + specular) * view.environmentIntensity * occlusion;
}

// The ShadowCascades view mode: the lit color tinted by the cascade each pixel reads. Some of
// the tint is added, so it also shows where the scene is dark.
float3
applyShadowCascadeTint(float3 color, float3 worldPosition, ViewData view)
{
  if (view.debugShadowIndex == kNoShadow) {
    return color;
  }
  const float viewDepth = mul(float4(worldPosition, 1.0f), view.view).z;
  const float4 tint =
      getShadowCascadeTint(view.shadowBufferIndex, view.debugShadowIndex, viewDepth);
  return lerp(color, color * tint.rgb + tint.rgb * 0.05f, tint.a);
}
