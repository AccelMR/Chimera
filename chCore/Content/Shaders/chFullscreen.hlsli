// A single triangle that covers the screen, for passes that work per pixel (tonemap,
// deferred lighting). It needs no vertex buffer: draw 3 vertices.
#pragma once

float4
fullscreenTrianglePosition(uint vertexId)
{
  // (-1, -1), (3, -1), (-1, 3): the screen fits inside.
  const float2 position = float2((vertexId << 1) & 2, vertexId & 2) * 2.0f - 1.0f;
  return float4(position, 0.0f, 1.0f);
}
