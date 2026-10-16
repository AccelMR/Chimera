// Takes the HDR scene color to the back buffer: exposure, ACES curve, then sRGB encoding.

#include "chColor.hlsli"
#include "chFullscreen.hlsli"

// Must match TonemapPushConstants in chTonemapPass.cpp.
struct PushConstants
{
  uint sceneColorIndex;
  float exposure;
  // 0 skips the ACES curve and only clamps (Renderer.Tonemap).
  uint applyCurve;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

float4
VSMain(uint vertexId : SV_VertexID) : SV_Position
{
  return fullscreenTrianglePosition(vertexId);
}

float4
PSMain(float4 position : SV_Position) : SV_Target0
{
  // Same size as the output, so each pixel reads its own texel without a sampler.
  Texture2D<float4> sceneColor = ResourceDescriptorHeap[g_push.sceneColorIndex];
  const float3 color = sceneColor.Load(int3(position.xy, 0)).rgb * g_push.exposure;
  const float3 mapped = g_push.applyCurve != 0 ? tonemapACES(color) : saturate(color);
  return float4(linearToSrgb(mapped), 1.0f);
}
