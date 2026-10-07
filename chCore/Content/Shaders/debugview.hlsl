// View modes that show scene data instead of the lit scene: the unlit texture, world
// normals, depth and texture coordinates, plus a flat color for wireframe lines.

#include "chBindless.hlsli"
#include "chTransforms.hlsli"

// Must match DebugMode in chDebugRenderPath.cpp.
static const uint kModeUnlit = 0;
static const uint kModeNormals = 1;
static const uint kModeDepth = 2;
static const uint kModeTexCoords = 3;
static const uint kModeColor = 4;

// Must match DebugCameraData in chDebugRenderPath.cpp.
struct CameraData
{
  row_major float4x4 view;
  row_major float4x4 projection;
  float nearPlane;
  float farPlane;
  float2 padding;
};

// Must match DebugPushConstants in chDebugRenderPath.cpp.
struct PushConstants
{
  row_major float4x4 model;
  float4 color;
  uint cameraIndex;
  uint textureIndex;
  uint samplerIndex;
  uint mode;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

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
  float viewDepth : TEXCOORD1;
};

VSOutput
VSMain(VSInput input)
{
  const CameraData camera = loadConstants<CameraData>(g_push.cameraIndex);
  const float4 worldPosition = mul(float4(input.position, 1.0f), g_push.model);
  const float4 viewPosition = mul(worldPosition, camera.view);

  VSOutput output;
  output.position = mul(viewPosition, camera.projection);
  output.normal = transformNormal(input.normal, g_push.model);
  output.texCoord = input.texCoord;
  // View space Z points forward.
  output.viewDepth = viewPosition.z;
  return output;
}

float4
PSMain(VSOutput input) : SV_Target0
{
  if (g_push.mode == kModeNormals) {
    return float4(normalize(input.normal) * 0.5f + 0.5f, 1.0f);
  }
  if (g_push.mode == kModeDepth) {
    // Logarithmic between the clip planes: a linear ramp over a far plane hundreds of units
    // away leaves everything near the camera black.
    const CameraData camera = loadConstants<CameraData>(g_push.cameraIndex);
    const float depth = log(max(input.viewDepth, camera.nearPlane) / camera.nearPlane) /
                        log(camera.farPlane / camera.nearPlane);
    return float4(saturate(depth).xxx, 1.0f);
  }
  if (g_push.mode == kModeTexCoords) {
    return float4(frac(input.texCoord), 0.0f, 1.0f);
  }
  if (g_push.mode == kModeColor) {
    return g_push.color;
  }

  Texture2D albedoTexture = ResourceDescriptorHeap[g_push.textureIndex];
  SamplerState albedoSampler = SamplerDescriptorHeap[g_push.samplerIndex];
  return albedoTexture.Sample(albedoSampler, input.texCoord);
}
