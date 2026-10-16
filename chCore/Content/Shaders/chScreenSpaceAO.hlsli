// What the ambient occlusion passes share: view space positions from the depth buffer.
#pragma once

#include "chView.hlsli"

// Distance along the view direction of the point a depth buffer value belongs to. Solved
// from the projection (depth = clip.z / clip.w), so it works with perspective and
// orthographic cameras.
float
linearizeDepth(float depth, ViewData view)
{
  const float4x4 projection = view.projection;
  return (projection[3][2] - depth * projection[3][3]) /
         (depth * projection[2][3] - projection[2][2]);
}

// View space position (X right, Y up, Z forward) of the point a texel shows; uv is the
// texel center in texture coordinates.
float3
viewPositionFromDepth(float2 uv, float depth, ViewData view)
{
  const float4x4 projection = view.projection;
  const float viewDepth = linearizeDepth(depth, view);
  const float clipW = viewDepth * projection[2][3] + projection[3][3];
  // The top row of pixels is NDC y = 1 with every graphics API the engine runs on.
  const float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
  const float x =
      (ndc.x * clipW - viewDepth * projection[2][0] - projection[3][0]) / projection[0][0];
  const float y =
      (ndc.y * clipW - viewDepth * projection[2][1] - projection[3][1]) / projection[1][1];
  return float3(x, y, viewDepth);
}

// Size in view units of one pixel of an image width pixels wide, at a view depth.
float
getViewPixelSize(float viewDepth, float width, ViewData view)
{
  const float4x4 projection = view.projection;
  const float clipW = viewDepth * projection[2][3] + projection[3][3];
  return 2.0f * clipW / (projection[0][0] * width);
}

// Weight of a neighbor in the blur and the scale up: 1 on the expected surface, 0 once it
// is further than tolerance (a fraction of the pixel's distance) from it.
float
getDepthWeight(float neighborDepth, float expectedDepth, float centerDepth, float tolerance)
{
  return saturate(1.0f - abs(neighborDepth - expectedDepth) / (centerDepth * tolerance));
}
