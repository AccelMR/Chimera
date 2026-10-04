/************************************************************************/
/**
 * @file chGraphicsTypes.cpp
 * @author AccelMR
 * @date 2026/10/03
 * @brief
 *  Graphics types and enums used in the graphics API.
 */
/************************************************************************/
#include "chGraphicsTypes.h"

#include "chHash.h"

namespace chEngineSDK {

/*
 */
uint64
GraphicsPipelineDesc::getHash() const
{
  uint64 hash = HashUtils::FNV_OFFSET_BASIS;
  hash = HashUtils::combine(hash, vertexShader.get());
  hash = HashUtils::combine(hash, fragmentShader.get());

  for (const VertexAttributeDesc& attribute : vertexLayout.getAttributes()) {
    hash = HashUtils::combine(hash, attribute.type);
    hash = HashUtils::combine(hash, attribute.format);
    hash = HashUtils::combine(hash, attribute.offset);
    hash = HashUtils::combine(hash, attribute.binding);
  }
  for (uint32 binding = 0; binding < vertexLayout.getBindingCount(); ++binding) {
    hash = HashUtils::combine(hash, vertexLayout.getStride(binding));
  }

  hash = HashUtils::combine(hash, topology);
  hash = HashUtils::combine(hash, raster.cullMode);
  hash = HashUtils::combine(hash, raster.frontFace);
  hash = HashUtils::combine(hash, raster.polygonMode);
  hash = HashUtils::combine(hash, raster.depthBiasConstant);
  hash = HashUtils::combine(hash, raster.depthBiasSlope);
  hash = HashUtils::combine(hash, depth.testEnable);
  hash = HashUtils::combine(hash, depth.writeEnable);
  hash = HashUtils::combine(hash, depth.compareOp);

  hash = HashUtils::combine(hash, colorAttachmentCount);
  for (uint32 i = 0; i < colorAttachmentCount; ++i) {
    const BlendAttachmentState& blend = blendStates[i];
    hash = HashUtils::combine(hash, colorFormats[i]);
    hash = HashUtils::combine(hash, blend.enable);
    hash = HashUtils::combine(hash, blend.srcColorFactor);
    hash = HashUtils::combine(hash, blend.dstColorFactor);
    hash = HashUtils::combine(hash, blend.colorOp);
    hash = HashUtils::combine(hash, blend.srcAlphaFactor);
    hash = HashUtils::combine(hash, blend.dstAlphaFactor);
    hash = HashUtils::combine(hash, blend.alphaOp);
  }
  hash = HashUtils::combine(hash, depthFormat);
  hash = HashUtils::combine(hash, samples);
  return hash;
}

} // namespace chEngineSDK
