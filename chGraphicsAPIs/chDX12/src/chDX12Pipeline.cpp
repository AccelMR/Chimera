/************************************************************************/
/**
 * @file chDX12Pipeline.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of IPipeline.
 */
/************************************************************************/
#include "chDX12Pipeline.h"

#include "chDX12API.h"
#include "chDX12Shader.h"
#include "chVertexLayout.h"

namespace chEngineSDK {
namespace {
struct VertexSemantic
{
  const ANSICHAR* name;
  UINT index;
};

// Direct3D 12 matches vertex attributes to shader inputs by semantic, so the HLSL inputs
// must use these names.
NODISCARD VertexSemantic
toSemantic(const VertexAttributeDesc& attribute)
{
  switch (attribute.type) {
  case VertexAttributeType::Position:
    return {"POSITION", 0};
  case VertexAttributeType::Normal:
    return {"NORMAL", 0};
  case VertexAttributeType::Color:
    return {"COLOR", 0};
  case VertexAttributeType::TexCoord0:
    return {"TEXCOORD", 0};
  case VertexAttributeType::TexCoord1:
    return {"TEXCOORD", 1};
  case VertexAttributeType::Tangent:
    return {"TANGENT", 0};
  case VertexAttributeType::Bitangent:
    return {"BINORMAL", 0};
  case VertexAttributeType::BoneIndices:
    return {"BLENDINDICES", 0};
  case VertexAttributeType::BoneWeights:
    return {"BLENDWEIGHT", 0};
  case VertexAttributeType::Custom:
  default:
    return {attribute.semanticName, 0};
  }
}

NODISCARD D3D12_BLEND
toBlend(BlendFactor factor)
{
  switch (factor) {
  case BlendFactor::Zero:
    return D3D12_BLEND_ZERO;
  case BlendFactor::One:
    return D3D12_BLEND_ONE;
  case BlendFactor::SrcColor:
    return D3D12_BLEND_SRC_COLOR;
  case BlendFactor::OneMinusSrcColor:
    return D3D12_BLEND_INV_SRC_COLOR;
  case BlendFactor::DstColor:
    return D3D12_BLEND_DEST_COLOR;
  case BlendFactor::OneMinusDstColor:
    return D3D12_BLEND_INV_DEST_COLOR;
  case BlendFactor::SrcAlpha:
    return D3D12_BLEND_SRC_ALPHA;
  case BlendFactor::OneMinusSrcAlpha:
    return D3D12_BLEND_INV_SRC_ALPHA;
  case BlendFactor::DstAlpha:
    return D3D12_BLEND_DEST_ALPHA;
  case BlendFactor::OneMinusDstAlpha:
  default:
    return D3D12_BLEND_INV_DEST_ALPHA;
  }
}

NODISCARD D3D12_BLEND_OP
toBlendOp(BlendOp op)
{
  switch (op) {
  case BlendOp::Subtract:
    return D3D12_BLEND_OP_SUBTRACT;
  case BlendOp::ReverseSubtract:
    return D3D12_BLEND_OP_REV_SUBTRACT;
  case BlendOp::Min:
    return D3D12_BLEND_OP_MIN;
  case BlendOp::Max:
    return D3D12_BLEND_OP_MAX;
  case BlendOp::Add:
  default:
    return D3D12_BLEND_OP_ADD;
  }
}

NODISCARD D3D12_CULL_MODE
toCullMode(CullMode cullMode)
{
  switch (cullMode) {
  case CullMode::None:
    return D3D12_CULL_MODE_NONE;
  case CullMode::Front:
    return D3D12_CULL_MODE_FRONT;
  case CullMode::Back:
  default:
    return D3D12_CULL_MODE_BACK;
  }
}

NODISCARD D3D12_PRIMITIVE_TOPOLOGY_TYPE
toTopologyType(PrimitiveTopology topology)
{
  switch (topology) {
  case PrimitiveTopology::PointList:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
  case PrimitiveTopology::LineList:
  case PrimitiveTopology::LineStrip:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  case PrimitiveTopology::TriangleList:
  case PrimitiveTopology::TriangleStrip:
  default:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  }
}

NODISCARD D3D_PRIMITIVE_TOPOLOGY
toTopology(PrimitiveTopology topology)
{
  switch (topology) {
  case PrimitiveTopology::PointList:
    return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
  case PrimitiveTopology::LineList:
    return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
  case PrimitiveTopology::LineStrip:
    return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
  case PrimitiveTopology::TriangleStrip:
    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
  case PrimitiveTopology::TriangleList:
  default:
    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  }
}

// One per attribute of a vertex layout; more than this is not a real mesh format.
constexpr uint32 kMaxVertexAttributes = 16;
} // namespace

/*
 */
DX12Pipeline::DX12Pipeline(ID3D12Device* device,
                           ID3D12RootSignature* rootSignature,
                           const GraphicsPipelineDesc& desc)
  : m_topology(toTopology(desc.topology))
{
  CH_ASSERT(desc.vertexShader && desc.fragmentShader);
  CH_ASSERT(desc.colorAttachmentCount <= GraphicsLimits::MAX_COLOR_ATTACHMENTS);

  const VertexLayout& vertexLayout = desc.vertexLayout;
  const auto& attributes = vertexLayout.getAttributes();
  CH_ASSERT(attributes.size() <= kMaxVertexAttributes);
  CH_ASSERT(vertexLayout.getBindingCount() <= MAX_VERTEX_BINDINGS);

  for (uint32 i = 0; i < vertexLayout.getBindingCount(); ++i) {
    m_strides[i] = vertexLayout.getStride(i);
  }

  Array<D3D12_INPUT_ELEMENT_DESC, kMaxVertexAttributes> inputElements{};
  for (uint32 i = 0; i < attributes.size(); ++i) {
    const VertexSemantic semantic = toSemantic(attributes[i]);
    inputElements[i] = {.SemanticName = semantic.name,
                        .SemanticIndex = semantic.index,
                        .Format = chVertexFormatToDxgiFormat(attributes[i].format),
                        .InputSlot = attributes[i].binding,
                        .AlignedByteOffset = attributes[i].offset,
                        .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                        .InstanceDataStepRate = 0};
  }

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
  pipelineDesc.pRootSignature = rootSignature;
  pipelineDesc.VS = static_cast<const DX12Shader&>(*desc.vertexShader).getBytecode();
  pipelineDesc.PS = static_cast<const DX12Shader&>(*desc.fragmentShader).getBytecode();

  // Each attachment has its own blend state, as the description lists them; the unused
  // ones keep the defaults, so every entry holds valid values.
  pipelineDesc.BlendState.IndependentBlendEnable = TRUE;
  for (uint32 i = 0; i < GraphicsLimits::MAX_COLOR_ATTACHMENTS; ++i) {
    const BlendAttachmentState blend =
        i < desc.colorAttachmentCount ? desc.blendStates[i] : BlendAttachmentState{};
    pipelineDesc.BlendState.RenderTarget[i] = {
        .BlendEnable = blend.enable ? TRUE : FALSE,
        .LogicOpEnable = FALSE,
        .SrcBlend = toBlend(blend.srcColorFactor),
        .DestBlend = toBlend(blend.dstColorFactor),
        .BlendOp = toBlendOp(blend.colorOp),
        .SrcBlendAlpha = toBlend(blend.srcAlphaFactor),
        .DestBlendAlpha = toBlend(blend.dstAlphaFactor),
        .BlendOpAlpha = toBlendOp(blend.alphaOp),
        .LogicOp = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL};
  }
  for (uint32 i = 0; i < desc.colorAttachmentCount; ++i) {
    pipelineDesc.RTVFormats[i] = chFormatToDxgiFormat(desc.colorFormats[i]);
  }
  pipelineDesc.SampleMask = 0xFFFFFFFFu;

  // Direct3D 12 takes the constant depth bias in whole units of the depth format.
  pipelineDesc.RasterizerState = {
      .FillMode = desc.raster.polygonMode == PolygonMode::Line ? D3D12_FILL_MODE_WIREFRAME
                                                               : D3D12_FILL_MODE_SOLID,
      .CullMode = toCullMode(desc.raster.cullMode),
      .FrontCounterClockwise = desc.raster.frontFace == FrontFace::CounterClockwise,
      .DepthBias = static_cast<INT>(desc.raster.depthBiasConstant),
      .DepthBiasClamp = 0.0f,
      .SlopeScaledDepthBias = desc.raster.depthBiasSlope,
      .DepthClipEnable = TRUE,
      .MultisampleEnable = desc.samples != SampleCount::Count1,
      .AntialiasedLineEnable = FALSE,
      .ForcedSampleCount = 0,
      .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF};

  const bool hasDepth = desc.depthFormat != Format::Unknown;
  pipelineDesc.DepthStencilState.DepthEnable = hasDepth && desc.depth.testEnable;
  pipelineDesc.DepthStencilState.DepthWriteMask = hasDepth && desc.depth.writeEnable
                                                      ? D3D12_DEPTH_WRITE_MASK_ALL
                                                      : D3D12_DEPTH_WRITE_MASK_ZERO;
  pipelineDesc.DepthStencilState.DepthFunc = chCompareOpToD3D12(desc.depth.compareOp);
  pipelineDesc.DepthStencilState.StencilEnable = FALSE;
  // Stencil is off, but the operations still need valid values.
  const D3D12_DEPTH_STENCILOP_DESC keepStencil{.StencilFailOp = D3D12_STENCIL_OP_KEEP,
                                               .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                                               .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                                               .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS};
  pipelineDesc.DepthStencilState.FrontFace = keepStencil;
  pipelineDesc.DepthStencilState.BackFace = keepStencil;

  pipelineDesc.InputLayout = {.pInputElementDescs = inputElements.data(),
                              .NumElements = static_cast<UINT>(attributes.size())};
  pipelineDesc.PrimitiveTopologyType = toTopologyType(desc.topology);
  pipelineDesc.NumRenderTargets = desc.colorAttachmentCount;
  pipelineDesc.DSVFormat = hasDepth ? chFormatToDxgiFormat(desc.depthFormat)
                                    : DXGI_FORMAT_UNKNOWN;
  pipelineDesc.SampleDesc = {.Count = static_cast<UINT>(desc.samples), .Quality = 0};

  DX12_CHECK(device->CreateGraphicsPipelineState(&pipelineDesc, IID_ID3D12PipelineState,
                                                 outPtr(m_pipelineState)));
}

/*
 */
DX12Pipeline::~DX12Pipeline()
{
  g_dx12API().getDeletionQueue().enqueue(m_pipelineState);
}

} // namespace chEngineSDK
