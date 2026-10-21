/************************************************************************/
/**
 * @file chTransientTexturePool.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Keeps the textures a render graph makes, so they are not created every frame.
 */
/************************************************************************/

#include "chTransientTexturePool.h"

#include "chIGraphicsAPI.h"
#include "chITexture.h"
#include "chITextureView.h"

namespace chEngineSDK {

/*
 */
uint32
TransientTexturePool::acquire(const RGTextureDesc& desc, TextureUsageFlags usage)
{
  for (uint32 i = 0; i < m_entries.size(); ++i) {
    Entry& entry = m_entries[i];
    if (entry.lastUsedFrame != m_frame && entry.desc == desc && entry.usage == usage) {
      entry.lastUsedFrame = m_frame;
      return i;
    }
  }

  Entry& entry = m_entries.emplace_back();
  entry.texture = IGraphicsAPI::instance().createTexture({.format = desc.format,
                                                          .width = desc.width,
                                                          .height = desc.height,
                                                          .mipLevels = desc.mipLevels,
                                                          .arrayLayers = desc.arrayLayers,
                                                          .samples = desc.samples,
                                                          .usage = usage});
  entry.view = entry.texture->createView();
  entry.desc = desc;
  entry.usage = usage;
  entry.lastUsedFrame = m_frame;
  return static_cast<uint32>(m_entries.size() - 1);
}

/*
 */
void
TransientTexturePool::nextFrame()
{
  for (SIZE_T i = m_entries.size(); i-- > 0;) {
    if (m_frame - m_entries[i].lastUsedFrame >= FRAMES_TO_KEEP) {
      if (i + 1 != m_entries.size()) {
        m_entries[i] = std::move(m_entries.back());
      }
      m_entries.pop_back();
    }
  }
  ++m_frame;
}

/*
 */
void
TransientTexturePool::clear()
{
  m_entries.clear();
}

} // namespace chEngineSDK
