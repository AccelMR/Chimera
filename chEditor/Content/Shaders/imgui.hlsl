// Dear ImGui geometry: 2D triangles in pixels, tinted by the vertex color and a texture
// (the font atlas or any texture the UI shows).

// Must match ImGuiRenderer's push constant struct.
struct PushConstants
{
  float2 scale;
  float2 translate;
  uint textureIndex;
  uint samplerIndex;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

// Must match ImDrawVert.
struct VSInput
{
  float2 position : POSITION;
  float2 texCoord : TEXCOORD0;
  float4 color : COLOR0;
};

struct VSOutput
{
  float4 position : SV_Position;
  float2 texCoord : TEXCOORD0;
  float4 color : COLOR0;
};

VSOutput
VSMain(VSInput input)
{
  VSOutput output;
  output.position = float4(input.position * g_push.scale + g_push.translate, 0.0f, 1.0f);
  output.texCoord = input.texCoord;
  output.color = input.color;
  return output;
}

float4
PSMain(VSOutput input) : SV_Target0
{
  Texture2D uiTexture = ResourceDescriptorHeap[g_push.textureIndex];
  SamplerState uiSampler = SamplerDescriptorHeap[g_push.samplerIndex];
  return input.color * uiTexture.Sample(uiSampler, input.texCoord);
}
