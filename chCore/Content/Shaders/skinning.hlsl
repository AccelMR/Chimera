// Moves the vertices of one skinned mesh by its bones into a buffer of the same layout, so
// every pass draws the result like any other mesh. One thread per vertex.

// Must match SkinningPushConstants in chSkinningPass.cpp.
struct PushConstants
{
  uint sourceIndex;
  uint skinIndex;
  uint matrixIndex;
  uint firstMatrix;
  uint outputIndex;
  uint vertexCount;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push : register(b0);

// VertexStatic: position, normal, texCoord, tangent (48 bytes).
static const uint kVertexSize = 48;
static const uint kNormalOffset = 12;
static const uint kTexCoordOffset = 24;
static const uint kTangentOffset = 32;
// VertexSkin: four bone indices then four weights, a byte each.
static const uint kSkinSize = 8;
static const uint kMatrixSize = 64;

// Loaded row by row, because a raw Load of a matrix may pick another layout.
float4x4
loadMatrix(ByteAddressBuffer matrices, uint index)
{
  const uint address = (g_push.firstMatrix + index) * kMatrixSize;
  return float4x4(asfloat(matrices.Load4(address)), asfloat(matrices.Load4(address + 16)),
                  asfloat(matrices.Load4(address + 32)),
                  asfloat(matrices.Load4(address + 48)));
}

[numthreads(64, 1, 1)]
void
CSMain(uint3 threadId : SV_DispatchThreadID)
{
  const uint vertex = threadId.x;
  if (vertex >= g_push.vertexCount) {
    return;
  }

  RWByteAddressBuffer source = ResourceDescriptorHeap[g_push.sourceIndex];
  RWByteAddressBuffer skins = ResourceDescriptorHeap[g_push.skinIndex];
  ByteAddressBuffer matrices = ResourceDescriptorHeap[g_push.matrixIndex];
  RWByteAddressBuffer output = ResourceDescriptorHeap[g_push.outputIndex];

  const uint2 skin = skins.Load2(vertex * kSkinSize);
  // Weights add up to 255, so the blended matrix keeps the scale of the bones.
  float4x4 blended = (float4x4)0;
  [unroll]
  for (uint influence = 0; influence < 4; ++influence) {
    const uint bone = (skin.x >> (influence * 8)) & 0xFF;
    const float weight = float((skin.y >> (influence * 8)) & 0xFF) / 255.0f;
    if (weight > 0.0f) {
      blended += loadMatrix(matrices, bone) * weight;
    }
  }

  const uint address = vertex * kVertexSize;
  const float3 position = asfloat(source.Load3(address));
  const float3 normal = asfloat(source.Load3(address + kNormalOffset));
  const float4 tangent = asfloat(source.Load4(address + kTangentOffset));

  // Normals take the matrix without its translation; bones with scale that differs per
  // axis would need its inverse transpose, which skins rarely have.
  const float3 skinnedPosition = mul(float4(position, 1.0f), blended).xyz;
  const float3 skinnedNormal = normalize(mul(float4(normal, 0.0f), blended).xyz);
  const float3 skinnedTangent = normalize(mul(float4(tangent.xyz, 0.0f), blended).xyz);

  // The texture coordinates are copied as they are.
  output.Store3(address, asuint(skinnedPosition));
  output.Store3(address + kNormalOffset, asuint(skinnedNormal));
  output.Store2(address + kTexCoordOffset, source.Load2(address + kTexCoordOffset));
  output.Store4(address + kTangentOffset, asuint(float4(skinnedTangent, tangent.w)));
}
