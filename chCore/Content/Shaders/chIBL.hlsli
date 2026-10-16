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
