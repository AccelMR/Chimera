// View modes that show scene data instead of the lit scene: the unlit texture, world
// normals, depth and texture coordinates, plus a flat color for wireframe lines.
// CH_VARIANT masked CH_ALPHA_MASK

#include "chBindless.hlsli"
#include "chColor.hlsli"
#include "chMaterial.hlsli"
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
  uint materialIndex;
  uint samplerIndex;
  uint mode;
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
  float viewDepth : TEXCOORD1;
  float4 tangent : TANGENT;
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
  output.tangent = transformTangent(input.tangent, g_push.model);
  // View space Z points forward.
  output.viewDepth = viewPosition.z;
  return output;
}

float4
PSMain(VSOutput input, bool isFrontFace : SV_IsFrontFace) : SV_Target0
{
  const MaterialData material = loadConstants<MaterialData>(g_push.materialIndex);
  SamplerState textureSampler = SamplerDescriptorHeap[g_push.samplerIndex];
#if defined(CH_ALPHA_MASK)
  // Every mode shows the shape the lit view shows, so the cut pixels are dropped first.
  clipAlphaMask(material, getBaseColor(material, textureSampler, input.texCoord).a);
#endif

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

  if (g_push.mode == kModeNormals) {
    // With the normal map, as the lighting sees it.
    const float3 normal =
        faceNormal(getSurfaceNormal(material, textureSampler, input.texCoord,
                                    normalize(input.normal), input.tangent),
                   isFrontFace);
    return float4(normal * 0.5f + 0.5f, 1.0f);
  }

  // Written straight to the back buffer, which holds sRGB values.
  const float4 albedo = getBaseColor(material, textureSampler, input.texCoord);
  return float4(linearToSrgb(albedo.rgb), albedo.a);
}
