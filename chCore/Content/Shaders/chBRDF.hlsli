// How a metallic-roughness surface reflects light: Cook-Torrance specular (GGX
// distribution, height-correlated Smith visibility, Schlick Fresnel) plus Lambert diffuse.
// Every lighting shader (forward and deferred) shares it, so both paths look the same.
#pragma once

static const float kPi = 3.14159265f;

// Dielectrics reflect about 4% of the light straight on.
static const float kDielectricReflectance = 0.04f;

// Below this the highlight of a point light becomes thinner than a pixel and flickers.
static const float kMinRoughness = 0.045f;

struct SurfaceData
{
  float3 baseColor;
  float metallic;
  float roughness;
  // Unit length, in the same space as the light and view directions.
  float3 normal;
};

float
distributionGGX(float normalDotHalf, float alpha)
{
  const float alphaSquared = alpha * alpha;
  const float denominator = normalDotHalf * normalDotHalf * (alphaSquared - 1.0f) + 1.0f;
  return alphaSquared / (kPi * denominator * denominator);
}

// The Smith geometry term already divided by the 4 * NdotL * NdotV of the Cook-Torrance
// denominator.
float
visibilitySmithGGXCorrelated(float normalDotView, float normalDotLight, float alpha)
{
  const float alphaSquared = alpha * alpha;
  const float viewTerm =
      normalDotLight *
      sqrt(normalDotView * normalDotView * (1.0f - alphaSquared) + alphaSquared);
  const float lightTerm =
      normalDotView *
      sqrt(normalDotLight * normalDotLight * (1.0f - alphaSquared) + alphaSquared);
  return 0.5f / max(viewTerm + lightTerm, 1e-5f);
}

float3
fresnelSchlick(float3 reflectance0, float viewDotHalf)
{
  const float factor = pow(1.0f - viewDotHalf, 5.0f);
  return reflectance0 + (1.0f - reflectance0) * factor;
}

// Light sent towards the eye for a light of color 1 coming from lightDirection, the cosine
// law included. Both directions point away from the surface and are unit length.
float3
evaluateBRDF(SurfaceData surface, float3 viewDirection, float3 lightDirection)
{
  const float normalDotLight = saturate(dot(surface.normal, lightDirection));
  if (normalDotLight <= 0.0f) {
    return float3(0.0f, 0.0f, 0.0f);
  }

  // Seen from the edge NdotV reaches zero and the visibility term divides by it.
  const float normalDotView = max(dot(surface.normal, viewDirection), 1e-4f);
  const float3 halfVector = normalize(viewDirection + lightDirection);
  const float normalDotHalf = saturate(dot(surface.normal, halfVector));
  const float viewDotHalf = saturate(dot(viewDirection, halfVector));

  // Roughness is authored perceptually; the distribution works with its square.
  const float roughness = max(surface.roughness, kMinRoughness);
  const float alpha = roughness * roughness;

  const float3 reflectance0 = lerp(kDielectricReflectance.xxx, surface.baseColor,
                                   surface.metallic);
  const float3 fresnel = fresnelSchlick(reflectance0, viewDotHalf);
  const float3 specular = distributionGGX(normalDotHalf, alpha) *
                          visibilitySmithGGXCorrelated(normalDotView, normalDotLight, alpha) *
                          fresnel;

  // Metals have no diffuse, and what the surface reflects as specular is not diffused.
  const float3 diffuse =
      (1.0f - fresnel) * surface.baseColor * (1.0f - surface.metallic) / kPi;

  return (diffuse + specular) * normalDotLight;
}
