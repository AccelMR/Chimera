// Material inputs read the same way by every shader that draws meshes. Each material keeps
// its factors and texture indexes in a constant buffer that comes by bindless index; a
// texture index of kNoTexture means the factor is used alone.
#pragma once

#include "chBindless.hlsli"

// Must match kNoTexture in chMaterial.cpp.
static const uint kNoTexture = 0xFFFFFFFF;

// Must match MaterialConstants in chMaterial.cpp.
struct MaterialData
{
  float4 baseColorFactor;
  float3 emissiveFactor;
  float metallicFactor;
  float roughnessFactor;
  float normalScale;
  float occlusionStrength;
  uint baseColorTexture;
  uint metallicRoughnessTexture;
  uint normalTexture;
  uint emissiveTexture;
  uint occlusionTexture;
  float alphaCutoff;
  float3 padding;
};

// White when the material has no texture there, so the factor is used alone. The index is
// the same for the whole draw, so it needs no NonUniformResourceIndex.
float4
sampleMaterialTexture(uint index, SamplerState textureSampler, float2 texCoord)
{
  if (index == kNoTexture) {
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
  }
  Texture2D materialTexture = ResourceDescriptorHeap[index];
  return materialTexture.Sample(textureSampler, texCoord);
}

float4
getBaseColor(MaterialData material, SamplerState textureSampler, float2 texCoord)
{
  return sampleMaterialTexture(material.baseColorTexture, textureSampler, texCoord) *
         material.baseColorFactor;
}

// Only the _masked variants call it: a shader that can discard loses the early depth test,
// so opaque materials are drawn with shaders that never do.
void
clipAlphaMask(MaterialData material, float alpha)
{
  clip(alpha - material.alphaCutoff);
}

// Back faces are only drawn for two-sided materials, and they are lit from their own side.
float3
faceNormal(float3 normal, bool isFrontFace)
{
  return isFrontFace ? normal : -normal;
}

// x metallic, y roughness. The texture keeps roughness in green and metallic in blue.
float2
getMetallicRoughness(MaterialData material, SamplerState textureSampler, float2 texCoord)
{
  const float4 texel =
      sampleMaterialTexture(material.metallicRoughnessTexture, textureSampler, texCoord);
  return float2(texel.b * material.metallicFactor, texel.g * material.roughnessFactor);
}

float
getOcclusion(MaterialData material, SamplerState textureSampler, float2 texCoord)
{
  const float occlusion =
      sampleMaterialTexture(material.occlusionTexture, textureSampler, texCoord).r;
  return lerp(1.0f, occlusion, material.occlusionStrength);
}

float3
getEmissive(MaterialData material, SamplerState textureSampler, float2 texCoord)
{
  return sampleMaterialTexture(material.emissiveTexture, textureSampler, texCoord).rgb *
         material.emissiveFactor;
}

// geometryNormal is unit length; tangent.w gives the side of the bitangent, which points up
// the image like the green of the normal map. All in the same space, which the result is in
// too.
float3
getSurfaceNormal(MaterialData material,
                 SamplerState textureSampler,
                 float2 texCoord,
                 float3 geometryNormal,
                 float4 tangent)
{
  if (material.normalTexture == kNoTexture) {
    return geometryNormal;
  }

  Texture2D normalTexture = ResourceDescriptorHeap[material.normalTexture];
  float3 tangentNormal = normalTexture.Sample(textureSampler, texCoord).xyz * 2.0f - 1.0f;
  tangentNormal.xy *= material.normalScale;

  // Interpolation shortens the vectors and tilts them apart, so the tangent is made unit and
  // perpendicular to the normal again.
  const float3 surfaceTangent =
      normalize(tangent.xyz - geometryNormal * dot(geometryNormal, tangent.xyz));
  const float3 bitangent = cross(geometryNormal, surfaceTangent) * tangent.w;
  return normalize(tangentNormal.x * surfaceTangent + tangentNormal.y * bitangent +
                   tangentNormal.z * geometryNormal);
}
