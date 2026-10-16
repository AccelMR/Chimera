// Draws the sky light's equirectangular image behind the scene: every pixel where nothing
// was drawn (depth still 1) shows the image in the direction of its view ray.

#include "chFullscreen.hlsli"

static const float kPi = 3.14159265f;

// Must match SkyPushConstants in chSkyPass.cpp.
// Flat, because a struct inside push constants is laid out differently by each target.
struct PushConstants
{
  // Rows of the matrix that takes a clip space position to a direction of the image, as
  // seen from the camera: inverse view rotation and projection, then the sky rotation.
  float4 clipToSkyRow0;
  float4 clipToSkyRow1;
  float4 clipToSkyRow2;
  float4 clipToSkyRow3;
  uint skyIndex;
  uint samplerIndex;
  uint depthIndex;
  float intensity;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

struct VertexOutput
{
  float4 position : SV_Position;
  float2 clipPosition : TEXCOORD0;
};

VertexOutput
VSMain(uint vertexId : SV_VertexID)
{
  VertexOutput output;
  output.position = fullscreenTrianglePosition(vertexId);
  output.clipPosition = output.position.xy;
  return output;
}

// Longitude around the up axis (Z) from forward (X) towards right (Y) runs along the
// width, latitude from up to down along the height.
float2
directionToEquirect(float3 direction)
{
  const float longitude = atan2(direction.y, direction.x);
  const float latitude = asin(clamp(direction.z, -1.0f, 1.0f));
  return float2(0.5f + longitude / (2.0f * kPi), 0.5f - latitude / kPi);
}

float4
PSMain(VertexOutput input) : SV_Target0
{
  Texture2D<float> depth = ResourceDescriptorHeap[g_push.depthIndex];
  if (depth.Load(int3(input.position.xy, 0)) < 1.0f) {
    discard;
  }

  const float4x4 clipToSky = float4x4(g_push.clipToSkyRow0, g_push.clipToSkyRow1,
                                      g_push.clipToSkyRow2, g_push.clipToSkyRow3);
  // Any depth works: every point of the ray has the same direction from the camera.
  const float4 rayPoint = mul(float4(input.clipPosition, 0.5f, 1.0f), clipToSky);
  const float3 direction = normalize(rayPoint.xyz / rayPoint.w);

  Texture2D<float4> sky = ResourceDescriptorHeap[g_push.skyIndex];
  SamplerState skySampler = SamplerDescriptorHeap[g_push.samplerIndex];
  // The longitude wraps at the back, where the derivatives jump, so mip 0 is read.
  const float3 color = sky.SampleLevel(skySampler, directionToEquirect(direction), 0.0f).rgb;
  return float4(color * g_push.intensity, 1.0f);
}
