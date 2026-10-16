// View modes that show one channel of the GBuffer, written straight to the back buffer.
// Pixels with nothing drawn keep the clear color.

#include "chBindless.hlsli"
#include "chColor.hlsli"
#include "chFullscreen.hlsli"
#include "chGBuffer.hlsli"

// Must match GBufferViewMode in chDeferredRenderPath.cpp.
static const uint kModeBaseColor = 0;
static const uint kModeMetallic = 1;
static const uint kModeRoughness = 2;
static const uint kModeOcclusion = 3;
static const uint kModeEmissive = 4;
static const uint kModeScreenSpaceAO = 5;

// Must match GraphicsLimits::INVALID_BINDLESS_INDEX.
static const uint kNoScreenSpaceAO = 0xFFFFFFFF;

// Must match GBufferViewPushConstants in chDeferredRenderPath.cpp.
// Flat, because a struct inside push constants is laid out differently by each target.
struct PushConstants
{
  uint baseColorIndex;
  uint normalIndex;
  uint surfaceIndex;
  uint emissiveIndex;
  uint depthIndex;
  uint mode;
  // kNoScreenSpaceAO when Renderer.SSAO is off or another mode is shown.
  uint screenSpaceAOIndex;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

GBufferTextures
getGBufferTextures()
{
  GBufferTextures textures;
  textures.baseColorIndex = g_push.baseColorIndex;
  textures.normalIndex = g_push.normalIndex;
  textures.surfaceIndex = g_push.surfaceIndex;
  textures.emissiveIndex = g_push.emissiveIndex;
  textures.depthIndex = g_push.depthIndex;
  return textures;
}

float4
VSMain(uint vertexId : SV_VertexID) : SV_Position
{
  return fullscreenTrianglePosition(vertexId);
}

float4
PSMain(float4 position : SV_Position) : SV_Target0
{
  const int3 texel = int3(position.xy, 0);
  const GBufferTextures textures = getGBufferTextures();
  if (loadGBufferDepth(textures, texel) >= 1.0f) {
    discard;
  }

  if (g_push.mode == kModeScreenSpaceAO) {
    if (g_push.screenSpaceAOIndex == kNoScreenSpaceAO) {
      return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    Texture2D<float> screenSpaceAO = ResourceDescriptorHeap[g_push.screenSpaceAOIndex];
    return float4(screenSpaceAO.Load(texel).xxx, 1.0f);
  }

  const GBufferSample gbuffer = loadGBuffer(textures, texel);
  // Colors are encoded for the back buffer; the scalar channels are data and are shown as
  // they are stored.
  if (g_push.mode == kModeMetallic) {
    return float4(gbuffer.surface.metallic.xxx, 1.0f);
  }
  if (g_push.mode == kModeRoughness) {
    return float4(gbuffer.surface.roughness.xxx, 1.0f);
  }
  if (g_push.mode == kModeOcclusion) {
    return float4(gbuffer.occlusion.xxx, 1.0f);
  }
  if (g_push.mode == kModeEmissive) {
    return float4(linearToSrgb(saturate(gbuffer.emissive)), 1.0f);
  }
  return float4(linearToSrgb(gbuffer.surface.baseColor), 1.0f);
}
