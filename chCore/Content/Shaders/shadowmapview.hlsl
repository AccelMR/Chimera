// The ShadowMaps view mode: the shadow atlases of the frame side by side, nearer depths
// brighter, with a line around each tile and unused tiles in dark blue.

#include "chFullscreen.hlsli"

// Must match ShadowMapViewPushConstants in chShadowRenderer.cpp.
struct PushConstants
{
  // Where each atlas is drawn, in pixels: x, y, width, height. A width of 0 hides it.
  float4 directionalRect;
  float4 spotRect;
  uint directionalIndex;
  uint spotIndex;
  uint directionalColumns;
  uint directionalTileCount;
  uint spotColumns;
  uint spotTileCount;
  uint2 padding;
  // Near plane over range of each spot map.
  float4 spotNearRatios[4];
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

static const float3 kBackground = float3(0.02f, 0.02f, 0.02f);
static const float3 kTileLine = float3(1.0f, 0.55f, 0.1f);
static const float3 kUnusedTile = float3(0.05f, 0.08f, 0.2f);

// A perspective depth back to 0 at the near plane and 1 at the far plane.
float
linearizePerspectiveDepth(float depth, float nearRatio)
{
  const float viewOverNear = 1.0f / max(1.0f - depth * (1.0f - nearRatio), 1e-6f);
  return saturate((viewOverNear - 1.0f) / max(1.0f / nearRatio - 1.0f, 1e-6f));
}

// The color of one pixel inside an atlas, or false outside its rectangle.
bool
shadeAtlas(float2 pixel,
           float4 rect,
           uint textureIndex,
           uint columns,
           uint tileCount,
           bool bPerspective,
           out float3 color)
{
  color = kBackground;
  const float2 local = (pixel - rect.xy) / rect.zw;
  if (rect.z <= 0.0f || any(local < 0.0f) || any(local >= 1.0f)) {
    return false;
  }

  Texture2D<float> atlas = ResourceDescriptorHeap[textureIndex];
  uint width;
  uint height;
  atlas.GetDimensions(width, height);
  const uint tileSize = width / max(columns, 1u);
  const uint2 texel = min(uint2(local * float2(width, height)), uint2(width, height) - 1u);
  const uint2 tile = texel / tileSize;
  const uint tileIndex = tile.y * columns + tile.x;

  // One screen pixel wide, whatever the zoom.
  const float2 texelsPerPixel = float2(width, height) / rect.zw;
  const float2 inTile = float2(texel % tileSize);
  if (any(inTile < texelsPerPixel) || any(inTile >= float(tileSize) - texelsPerPixel)) {
    color = kTileLine;
    return true;
  }
  if (tileIndex >= tileCount) {
    color = kUnusedTile;
    return true;
  }

  const float depth = atlas.Load(int3(texel, 0));
  float distance = depth;
  if (bPerspective) {
    const float nearRatio = g_push.spotNearRatios[tileIndex / 4][tileIndex % 4];
    distance = linearizePerspectiveDepth(depth, nearRatio);
  }
  color = (1.0f - distance).xxx;
  return true;
}

float4
VSMain(uint vertexId : SV_VertexID) : SV_Position
{
  return fullscreenTrianglePosition(vertexId);
}

float4
PSMain(float4 position : SV_Position) : SV_Target0
{
  float3 color;
  if (shadeAtlas(position.xy, g_push.directionalRect, g_push.directionalIndex,
                 g_push.directionalColumns, g_push.directionalTileCount, false, color)) {
    return float4(color, 1.0f);
  }
  shadeAtlas(position.xy, g_push.spotRect, g_push.spotIndex, g_push.spotColumns,
             g_push.spotTileCount, true, color);
  return float4(color, 1.0f);
}
