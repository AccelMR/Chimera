// One mip of the environment cube from the one above it: each texel averages the 2x2
// texels it covers on the same face. The filtered importance sampling of the irradiance
// and prefiltered cubes reads these blurred mips.

// Must match DownsamplePushConstants in chImageBasedLighting.cpp.
struct PushConstants
{
  uint sourceIndex;
  uint outputIndex;
  uint size;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

[numthreads(8, 8, 1)]
void
CSMain(uint3 threadId : SV_DispatchThreadID)
{
  if (any(threadId.xy >= g_push.size)) {
    return;
  }

  RWTexture2DArray<float4> source = ResourceDescriptorHeap[g_push.sourceIndex];
  const uint3 corner = uint3(threadId.xy * 2, threadId.z);
  const float4 sum = source[corner] + source[corner + uint3(1, 0, 0)] +
                     source[corner + uint3(0, 1, 0)] + source[corner + uint3(1, 1, 0)];

  RWTexture2DArray<float4> output = ResourceDescriptorHeap[g_push.outputIndex];
  output[threadId] = sum * 0.25f;
}
