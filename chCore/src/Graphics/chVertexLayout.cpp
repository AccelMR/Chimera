/************************************************************************/
/**
 * @file chVertexLayout.h
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vertex layout class. This class is used to define the vertex layout
 * for the graphics pipeline. It is used to describe the format of the
 * vertex data and how it is laid out in memory.
 */
/************************************************************************/

#include "chVertexLayout.h"

#include "chLogger.h"
#include "chMath.h"
#include "chStringUtils.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(VertexLayoutLog, All);

/*
*/
VertexLayout
VertexLayout::createPostionColorLayout() {
  VertexLayout layout;
  layout.addAttribute(VertexAttributeType::Position, VertexFormat::Float3);
  layout.addAttribute(VertexAttributeType::Color, VertexFormat::Float4);
  return layout;
}

/*
*/
VertexLayout
VertexLayout::createPositionNormalTexCoordLayout() {
  VertexLayout layout;
  layout.addAttribute(VertexAttributeType::Position, VertexFormat::Float3);
  layout.addAttribute(VertexAttributeType::Normal, VertexFormat::Float3);
  layout.addAttribute(VertexAttributeType::TexCoord0, VertexFormat::Float2);
  return layout;
}

/*
*/
void
VertexLayout::addAttribute(VertexAttributeType type,
             VertexFormat format,
             uint32 offset /*= UINT32_MAX*/,
             uint32 binding /*= 0*/) {
  VertexAttributeDesc desc{
    .type = type,
    .format = format,
    .offset = (offset == UINT32_MAX) ? calculateOffset(binding) : offset,
    .binding = binding,
    .semanticName = ""
  };

  m_attributes.push_back(desc);

  updateSizes(binding, desc.offset, format);
}

/*
*/
void
VertexLayout::addCustomAttribute(const String& semanticName,
                                 VertexFormat format,
                                 uint32 offset /*= UINT32_MAX*/,
                                 uint32 binding/* = 0*/) {

  if (semanticName.size() >= sizeof(semanticName)) {
    CH_LOG(VertexLayoutLog, Warning,
      "Semantic name '{0}' is too long for custom attribute. It will be truncated.",
      semanticName);
  }
  VertexAttributeDesc desc{
    .type = VertexAttributeType::Custom,
    .format = format,
    .offset = (offset == UINT32_MAX) ? calculateOffset(binding) : offset,
    .binding = binding,
    .semanticName = ""
  };
  chString::copyToANSI(desc.semanticName, semanticName, sizeof(desc.semanticName) - 1);

  m_attributes.push_back(desc);

  updateSizes(binding, desc.offset, format);
}

/*
*/
void
VertexLayout::updateSizes(uint32 binding, uint32 offset, VertexFormat format) {
  if (binding == 0) {
    m_vertexSize = Math::max(m_vertexSize, offset + getFormatSize(format));
  }

  if (binding >= m_strides.size()) {
    m_strides.resize(binding + 1, 0);
  }

  m_strides[binding] = Math::max(m_strides[binding],
                                 offset + getFormatSize(format));
}

/*
*/
uint32
VertexLayout::calculateOffset(uint32 binding) const {
  if (binding >= m_strides.size()) {
    return 0;
  }
  return m_strides[binding];
}

/*
*/
uint32
VertexLayout::getFormatSize(VertexFormat format) {
  switch (format) {
    case VertexFormat::Float: return sizeof(float);
    case VertexFormat::Float2: return sizeof(float) * 2;
    case VertexFormat::Float3: return sizeof(float) * 3;
    case VertexFormat::Float4: return sizeof(float) * 4;
    case VertexFormat::Int: return sizeof(int);
    case VertexFormat::Int2: return sizeof(int) * 2;
    case VertexFormat::Int3: return sizeof(int) * 3;
    case VertexFormat::Int4: return sizeof(int) * 4;
    case VertexFormat::Short2: return sizeof(short) * 2;
    case VertexFormat::Short4: return sizeof(short) * 4;
    default: return 0;
  }
}
} // namespace chEngineSDK
