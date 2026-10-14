// Mesh lit by one fixed directional light, with its material's normal map, occlusion and
// emissive.
// Matrices come from the engine stored row by row, and the engine uses row vectors,
// so they are declared row_major and applied as mul(vector, matrix).

#include "chBindless.hlsli"
#include "chMaterial.hlsli"
#include "chTransforms.hlsli"

struct CameraData
{
  row_major float4x4 view;
  row_major float4x4 projection;
};

// Must match DrawPushConstants in chForwardRenderPath.cpp.
struct PushConstants
{
  row_major float4x4 model;
  uint cameraIndex;
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
  const CameraData camera = loadConstants<CameraData>(g_push.cameraIndex);
  const float4 worldPosition = mul(float4(input.position, 1.0f), g_push.model);

  VSOutput output;
  output.worldPosition = worldPosition.xyz;
  output.position = mul(mul(worldPosition, camera.view), camera.projection);
  output.normal = transformNormal(input.normal, g_push.model);
  output.texCoord = input.texCoord;
  output.tangent = transformTangent(input.tangent, g_push.model);
  return output;
}

float4
PSMain(VSOutput input) : SV_Target0
{
  const MaterialData material = loadConstants<MaterialData>(g_push.materialIndex);
  SamplerState textureSampler = SamplerDescriptorHeap[g_push.samplerIndex];

  const float4 baseColor = getBaseColor(material, textureSampler, input.texCoord);
  const float3 normal = getSurfaceNormal(material, textureSampler, input.texCoord,
                                         normalize(input.normal), input.tangent);
  const float occlusion = getOcclusion(material, textureSampler, input.texCoord);
  const float3 emissive = getEmissive(material, textureSampler, input.texCoord);

  const float3 lightDirection = normalize(float3(1.0f, 1.0f, 1.0f));
  const float diffuse = max(dot(normal, lightDirection), 0.0f);
  const float3 ambient = float3(0.1f, 0.1f, 0.1f) * occlusion;

  return float4(baseColor.rgb * (ambient + diffuse) + emissive, baseColor.a);
}
