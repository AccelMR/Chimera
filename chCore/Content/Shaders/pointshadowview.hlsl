// The point light part of the ShadowMaps view mode: the six faces of each point light's cube
// map in one row per light (+X, -X, +Y, -Y, +Z, -Z), nearer depths brighter, with a line
// around each face.

#include "chFullscreen.hlsli"

// Must match PointShadowViewPushConstants in chShadowRenderer.cpp.
struct PushConstants
{
  // Where the faces are drawn, in pixels: x, y, width, height.
  float4 rect;
  uint layerIndex;
  uint cubeCount;
  uint2 padding;
  // Near plane over range of each cube.
  float4 nearRatios[2];
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

static const uint kFaceCount = 6;
static const float3 kFaceLine = float3(1.0f, 0.55f, 0.1f);

// A perspective depth back to 0 at the near plane and 1 at the far plane.
float
linearizePerspectiveDepth(float depth, float nearRatio)
{
  const float viewOverNear = 1.0f / max(1.0f - depth * (1.0f - nearRatio), 1e-6f);
  return saturate((viewOverNear - 1.0f) / max(1.0f / nearRatio - 1.0f, 1e-6f));
}

float4
VSMain(uint vertexId : SV_VertexID) : SV_Position
{
  return fullscreenTrianglePosition(vertexId);
}

float4
PSMain(float4 position : SV_Position) : SV_Target0
{
  const float4 rect = g_push.rect;
  const float2 local = (position.xy - rect.xy) / rect.zw;
  if (rect.z <= 0.0f || any(local < 0.0f) || any(local >= 1.0f)) {
    discard;
  }

  Texture2DArray<float> faces = ResourceDescriptorHeap[g_push.layerIndex];
  uint faceSize;
  uint faceHeight;
  uint layerCount;
  faces.GetDimensions(faceSize, faceHeight, layerCount);

  const float2 grid = float2(kFaceCount, g_push.cubeCount);
  const float2 cell = local * grid;
  const uint2 face = min(uint2(cell), uint2(grid) - 1u);
  const float2 inFace = cell - float2(face);

  // One screen pixel wide, whatever the zoom.
  const float2 cellsPerPixel = grid / rect.zw;
  if (any(inFace < cellsPerPixel) || any(inFace >= 1.0f - cellsPerPixel)) {
    return float4(kFaceLine, 1.0f);
  }

  const uint cube = face.y;
  const uint2 texel = min(uint2(inFace * faceSize), uint2(faceSize, faceSize) - 1u);
  const float depth = faces.Load(int4(texel, cube * kFaceCount + face.x, 0));
  const float nearRatio = g_push.nearRatios[cube / 4][cube % 4];
  return float4((1.0f - linearizePerspectiveDepth(depth, nearRatio)).xxx, 1.0f);
}
