// Second pass of the deferred path: lights every pixel the GBuffer covers with the same
// BRDF and lights as the forward path, so both look the same. Pixels with nothing drawn
// keep the clear color.

#include "chBindless.hlsli"
#include "chFullscreen.hlsli"
#include "chGBuffer.hlsli"
#include "chLights.hlsli"
#include "chView.hlsli"

// Must match LightingPushConstants in chDeferredRenderPath.cpp.
// Flat, because a struct inside push constants is laid out differently by each target.
struct PushConstants
{
  uint baseColorIndex;
  uint normalIndex;
  uint surfaceIndex;
  uint emissiveIndex;
  uint depthIndex;
  uint viewIndex;
  float2 inverseSize;
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
  const float depth = loadGBufferDepth(textures, texel);
  if (depth >= 1.0f) {
    discard;
  }

  const ViewData view = loadConstants<ViewData>(g_push.viewIndex);
  const GBufferSample gbuffer = loadGBuffer(textures, texel);

  // The top row of pixels is NDC y = 1 with every graphics API the engine runs on.
  const float2 screen = position.xy * g_push.inverseSize;
  const float4 clip = float4(screen.x * 2.0f - 1.0f, 1.0f - screen.y * 2.0f, depth, 1.0f);
  const float4 world = mul(clip, view.inverseViewProjection);
  const float3 worldPosition = world.xyz / world.w;

  const float3 viewDirection = normalize(view.cameraPosition - worldPosition);
  float3 color = evaluateLights(gbuffer.surface, worldPosition, viewDirection, view);
  color += evaluateAmbient(gbuffer.surface, viewDirection, gbuffer.occlusion, view);
  color += gbuffer.emissive;
  color = applyShadowCascadeTint(color, worldPosition, view);
  return float4(color, 1.0f);
}
