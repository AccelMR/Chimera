// Sampling helpers of the image based lighting, shared by the shaders that integrate the
// environment over the GGX lobe.
#pragma once

#include "chBRDF.hlsli"

// Point i of n of a low discrepancy sequence in [0, 1)^2: evenly spread samples converge
// with far fewer of them than random ones.
float2
hammersley(uint i, uint n)
{
  return float2((float)i / (float)n, (float)reversebits(i) * 2.3283064365386963e-10f);
}

// Half vector around +Z picked with the probability of the GGX distribution, so the
// samples gather where the lobe reflects most.
float3
importanceSampleGGX(float2 xi, float alpha)
{
  const float phi = 2.0f * kPi * xi.x;
  const float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (alpha * alpha - 1.0f) * xi.y));
  const float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
  return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Direction around +Z picked with the probability of the cosine (pdf = cos / pi), the
// weight of diffuse light.
float3
cosineSampleHemisphere(float2 xi)
{
  const float phi = 2.0f * kPi * xi.x;
  const float cosTheta = sqrt(1.0f - xi.y);
  const float sinTheta = sqrt(xi.y);
  return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Takes a direction around +Z to one around the unit vector normal.
float3
tangentToWorld(float3 direction, float3 normal)
{
  const float3 up = abs(normal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
  const float3 tangent = normalize(cross(up, normal));
  const float3 bitangent = cross(normal, tangent);
  return tangent * direction.x + bitangent * direction.y + normal * direction.z;
}

// Where a direction falls on an equirectangular image: the longitude around the up axis
// (Z), from forward (X) towards right (Y), runs along the width, the latitude from up to
// down along the height. The sky background and the environment cube both use it.
float2
directionToEquirect(float3 direction)
{
  const float longitude = atan2(direction.y, direction.x);
  const float latitude = asin(clamp(direction.z, -1.0f, 1.0f));
  return float2(0.5f + longitude / (2.0f * kPi), 0.5f - latitude / kPi);
}

// Direction through the center of a texel of a cube face, as cube sampling picks faces
// (the same table in Direct3D and Vulkan): face 0 to 5 is +X, -X, +Y, -Y, +Z, -Z, and the
// texel position goes right and down the face.
float3
cubeTexelDirection(uint3 texel, uint size)
{
  const float2 position = (float2(texel.xy) + 0.5f) / size * 2.0f - 1.0f;
  const float s = position.x;
  const float t = position.y;
  float3 direction;
  switch (texel.z) {
    case 0: direction = float3(1.0f, -t, -s); break;
    case 1: direction = float3(-1.0f, -t, s); break;
    case 2: direction = float3(s, 1.0f, t); break;
    case 3: direction = float3(s, -1.0f, -t); break;
    case 4: direction = float3(s, -t, 1.0f); break;
    default: direction = float3(-s, -t, -1.0f); break;
  }
  return normalize(direction);
}

// Mip of a cube with faces of cubeSize texels whose texels cover the solid angle a sample
// of probability pdf stands for, out of sampleCount. Reading that blurred mip instead of
// mip 0 is what keeps few samples from showing bright dots (filtered importance sampling).
float
sampleMipLevel(float pdf, uint sampleCount, uint cubeSize)
{
  const float sampleSolidAngle = 1.0f / (sampleCount * max(pdf, 1e-6f));
  const float texelSolidAngle = 4.0f * kPi / (6.0f * cubeSize * cubeSize);
  return max(0.5f * log2(sampleSolidAngle / texelSolidAngle) + 1.0f, 0.0f);
}
