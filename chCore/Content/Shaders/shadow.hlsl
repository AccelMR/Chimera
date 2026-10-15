// Draws the depth of the shadow casters as a light sees them. Opaque materials need no pixel
// shader; masked ones cut their holes, so leaves and fences cast the shape they show.
// Matrices are row_major and applied as mul(vector, matrix), as in the engine.
// CH_VARIANT masked CH_ALPHA_MASK

#include "chBindless.hlsli"
#include "chMaterial.hlsli"

// Must match ShadowPushConstants in chShadowRenderer.cpp.
struct PushConstants
{
  // Model times the light's view projection.
  row_major float4x4 modelViewProjection;
  uint materialIndex;
  uint samplerIndex;
  uint2 padding;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

// Every attribute of the vertex is declared, because Vulkan matches them by order.
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
  float2 texCoord : TEXCOORD0;
};

VSOutput
VSMain(VSInput input)
{
  VSOutput output;
  output.position = mul(float4(input.position, 1.0f), g_push.modelViewProjection);
  output.texCoord = input.texCoord;
  return output;
}

// The build makes every entry point it finds for every variant, so the opaque one exists
// too, empty; the renderer draws opaque casters without a pixel shader.
void
PSMain(VSOutput input)
{
#if defined(CH_ALPHA_MASK)
  const MaterialData material = loadConstants<MaterialData>(g_push.materialIndex);
  SamplerState textureSampler = SamplerDescriptorHeap[g_push.samplerIndex];
  clipAlphaMask(material, getBaseColor(material, textureSampler, input.texCoord).a);
#endif
}
