// The scene lights a lighting shader loops over. They come in a raw buffer read by bindless
// index, one LightData after another.
#pragma once

#include "chBRDF.hlsli"

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
  float3 padding;
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

// Light sent towards the eye by every light of the buffer.
float3
evaluateLights(SurfaceData surface,
               float3 worldPosition,
               float3 viewDirection,
               uint lightBufferIndex,
               uint lightCount)
{
  ByteAddressBuffer lights = ResourceDescriptorHeap[lightBufferIndex];
  float3 result = float3(0.0f, 0.0f, 0.0f);
  for (uint i = 0; i < lightCount; ++i) {
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

    if (attenuation > 0.0f) {
      result += evaluateBRDF(surface, viewDirection, lightDirection) * light.color *
                attenuation;
    }
  }
  return result;
}

// The constant light from all sides that stands in for the environment. Only diffuse:
// without an environment there is nothing for it to reflect.
float3
evaluateAmbient(SurfaceData surface, float ambient, float occlusion)
{
  return surface.baseColor * (1.0f - surface.metallic) * ambient * occlusion;
}
