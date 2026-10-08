// Takes the HDR scene color to the back buffer: exposure, ACES curve, then sRGB encoding.
// A single triangle that covers the screen, so it needs no vertex buffer.

#include "chColor.hlsli"

// Must match TonemapPushConstants in chTonemapPass.cpp.
struct PushConstants
{
  uint sceneColorIndex;
  float exposure;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

float4
VSMain(uint vertexId : SV_VertexID) : SV_Position
{
  // (-1, -1), (3, -1), (-1, 3): the screen fits inside.
  const float2 position = float2((vertexId << 1) & 2, vertexId & 2) * 2.0f - 1.0f;
  return float4(position, 0.0f, 1.0f);
}

float4
PSMain(float4 position : SV_Position) : SV_Target0
{
  // Same size as the output, so each pixel reads its own texel without a sampler.
  Texture2D<float4> sceneColor = ResourceDescriptorHeap[g_push.sceneColorIndex];
  const float3 color = sceneColor.Load(int3(position.xy, 0)).rgb * g_push.exposure;
  return float4(linearToSrgb(tonemapACES(color)), 1.0f);
}
