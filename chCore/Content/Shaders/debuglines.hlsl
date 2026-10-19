// Lines of DebugDraw. There is no vertex buffer: each vertex reads its position and color
// from a storage buffer by its index.

#include "chColor.hlsli"

// Must match DebugLineVertex in chDebugDraw.h.
static const uint kVertexStride = 16;

// Must match DebugLinePushConstants in chDebugLinePass.cpp.
struct PushConstants
{
  row_major float4x4 viewProjection;
  uint vertexBufferIndex;
  // The draw always starts at vertex 0, so where its lines start in the buffer comes here.
  uint firstVertex;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

struct VSOutput
{
  float4 position : SV_Position;
  float4 color : COLOR0;
};

VSOutput
VSMain(uint vertexId : SV_VertexID)
{
  ByteAddressBuffer vertices = ResourceDescriptorHeap[g_push.vertexBufferIndex];
  const uint address = (g_push.firstVertex + vertexId) * kVertexStride;
  const float3 position = asfloat(vertices.Load3(address));
  const uint color = vertices.Load(address + 12);

  VSOutput output;
  output.position = mul(float4(position, 1.0f), g_push.viewProjection);
  output.color = float4(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF, color >> 24) /
                 255.0f;
  return output;
}

float4
PSMain(VSOutput input) : SV_Target0
{
  // Drawn after the tonemap, straight into the sRGB values of the output.
  return float4(linearToSrgb(input.color.rgb), input.color.a);
}
