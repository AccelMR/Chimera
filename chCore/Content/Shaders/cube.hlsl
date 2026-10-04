// Textured mesh with one fixed directional light.
// Matrices come from the engine stored row by row, and the engine uses row vectors,
// so they are declared row_major and applied as mul(vector, matrix).

struct ProjectionViewModel
{
  row_major float4x4 projection;
  row_major float4x4 view;
  row_major float4x4 model;
};

[[vk::binding(0)]] ConstantBuffer<ProjectionViewModel> pvm : register(b0);

// Same binding for both: the Vulkan pipeline uses one combined image sampler here.
[[vk::combinedImageSampler]] [[vk::binding(1)]] Texture2D albedoTexture : register(t0);
[[vk::combinedImageSampler]] [[vk::binding(1)]] SamplerState albedoSampler : register(s0);

struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
  float4 position : SV_Position;
  float3 normal : NORMAL;
  float2 texCoord : TEXCOORD0;
  float3 worldPosition : TEXCOORD1;
};

VSOutput
VSMain(VSInput input)
{
  const float4 worldPosition = mul(float4(input.position, 1.0f), pvm.model);

  VSOutput output;
  output.worldPosition = worldPosition.xyz;
  output.position = mul(mul(worldPosition, pvm.view), pvm.projection);
  output.normal = mul(input.normal, (float3x3)pvm.model);
  output.texCoord = input.texCoord;
  return output;
}

float4
PSMain(VSOutput input) : SV_Target0
{
  const float3 lightDirection = normalize(float3(1.0f, 1.0f, 1.0f));
  const float diffuse = max(dot(normalize(input.normal), lightDirection), 0.0f);
  const float3 ambient = float3(0.1f, 0.1f, 0.1f);

  const float4 textureColor = albedoTexture.Sample(albedoSampler, input.texCoord);
  return float4(ambient + diffuse, 1.0f) * textureColor;
}
