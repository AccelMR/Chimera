// Mesh lit by the scene lights with its metallic-roughness material, plus a constant ambient.
// Matrices come from the engine stored row by row, and the engine uses row vectors,
// so they are declared row_major and applied as mul(vector, matrix).
// CH_VARIANT masked CH_ALPHA_MASK

#include "chBindless.hlsli"
#include "chLights.hlsli"
#include "chMaterial.hlsli"
#include "chTransforms.hlsli"
#include "chView.hlsli"

// Must match DrawPushConstants in chForwardRenderPath.cpp.
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
  float3 worldPosition : TEXCOORD1;
  float4 tangent : TANGENT;
};

VSOutput
VSMain(VSInput input)
{
  const ViewData view = loadConstants<ViewData>(g_push.viewIndex);
  const float4 worldPosition = mul(float4(input.position, 1.0f), g_push.model);

  VSOutput output;
  output.worldPosition = worldPosition.xyz;
  output.position = mul(mul(worldPosition, view.view), view.projection);
  output.normal = transformNormal(input.normal, g_push.model);
  output.texCoord = input.texCoord;
  output.tangent = transformTangent(input.tangent, g_push.model);
  return output;
}

float4
PSMain(VSOutput input, bool isFrontFace : SV_IsFrontFace) : SV_Target0
{
  const ViewData view = loadConstants<ViewData>(g_push.viewIndex);
  const MaterialData material = loadConstants<MaterialData>(g_push.materialIndex);
  SamplerState textureSampler = SamplerDescriptorHeap[g_push.samplerIndex];

  const float4 baseColor = getBaseColor(material, textureSampler, input.texCoord);
#if defined(CH_ALPHA_MASK)
  clipAlphaMask(material, baseColor.a);
#endif
  const float2 metallicRoughness =
      getMetallicRoughness(material, textureSampler, input.texCoord);

  SurfaceData surface;
  surface.baseColor = baseColor.rgb;
  surface.metallic = metallicRoughness.x;
  surface.roughness = metallicRoughness.y;
  surface.normal = faceNormal(getSurfaceNormal(material, textureSampler, input.texCoord,
                                               normalize(input.normal), input.tangent),
                              isFrontFace);

  const float3 viewDirection = normalize(view.cameraPosition - input.worldPosition);
  float3 color = evaluateLights(surface, input.worldPosition, viewDirection,
                                view.lightBufferIndex, view.lightCount);
  color += evaluateAmbient(surface, view.ambient,
                           getOcclusion(material, textureSampler, input.texCoord));
  color += getEmissive(material, textureSampler, input.texCoord);
  return float4(color, baseColor.a);
}
