// First pass of the deferred path: writes what the lighting needs of each visible surface
// into the GBuffer, so every light is evaluated once per pixel instead of once per drawn
// fragment. Matrices are row_major and applied as mul(vector, matrix), as in the engine.

#include "chBindless.hlsli"
#include "chMaterial.hlsli"
#include "chTransforms.hlsli"
#include "chView.hlsli"

// Must match GBufferPushConstants in chDeferredRenderPath.cpp.
struct PushConstants
{
  row_major float4x4 model;
  uint viewIndex;
  uint materialIndex;
  uint samplerIndex;
  uint padding;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 texCoord : TEXCOORD0;
  float4 tangent : TANGENT;
};

struct VSOutput
{
  float4 position : SV_Position;
  float3 normal : NORMAL;
  float2 texCoord : TEXCOORD0;
  float4 tangent : TANGENT;
};

// The formats are in chDeferredRenderPath.cpp.
struct PSOutput
{
  // Linear here; the sRGB target encodes it.
  float4 baseColor : SV_Target0;
  // World normal moved from -1..1 to 0..1.
  float4 normal : SV_Target1;
  // Metallic, roughness, occlusion.
  float4 surface : SV_Target2;
  float4 emissive : SV_Target3;
};

VSOutput
VSMain(VSInput input)
{
  const ViewData view = loadConstants<ViewData>(g_push.viewIndex);
  const float4 worldPosition = mul(float4(input.position, 1.0f), g_push.model);

  VSOutput output;
  output.position = mul(mul(worldPosition, view.view), view.projection);
  output.normal = transformNormal(input.normal, g_push.model);
  output.texCoord = input.texCoord;
  output.tangent = transformTangent(input.tangent, g_push.model);
  return output;
}

PSOutput
PSMain(VSOutput input)
{
  const MaterialData material = loadConstants<MaterialData>(g_push.materialIndex);
  SamplerState textureSampler = SamplerDescriptorHeap[g_push.samplerIndex];

  const float2 metallicRoughness =
      getMetallicRoughness(material, textureSampler, input.texCoord);
  const float3 normal = getSurfaceNormal(material, textureSampler, input.texCoord,
                                         normalize(input.normal), input.tangent);

  PSOutput output;
  output.baseColor = float4(getBaseColor(material, textureSampler, input.texCoord).rgb, 1.0f);
  output.normal = float4(normal * 0.5f + 0.5f, 1.0f);
  output.surface = float4(metallicRoughness,
                          getOcclusion(material, textureSampler, input.texCoord), 1.0f);
  output.emissive = float4(getEmissive(material, textureSampler, input.texCoord), 1.0f);
  return output;
}
