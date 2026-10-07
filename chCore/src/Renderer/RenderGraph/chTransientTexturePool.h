/************************************************************************/
/**
 * @file chTransientTexturePool.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Keeps the textures a render graph makes, so they are not created every frame.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chRenderGraph.h"

namespace chEngineSDK {

/**
 * Exists so the textures a RenderGraph asks for each frame are made once: a request takes a
 * free texture with the same description and usage, and a texture nobody asked for in
 * FRAMES_TO_KEEP frames (a size left behind by a resize) is released. Each texture keeps
 * the state the last frame left it in, which the graph needs for its first barrier.
 */
class CH_CORE_EXPORT TransientTexturePool
{
 public:
  struct Entry
  {
    SPtr<ITexture> texture;
    SPtr<ITextureView> view;
    RGTextureDesc desc;
    TextureUsageFlags usage = TextureUsage::NoneUsage;
    ResourceState state = ResourceState::Undefined;
    uint64 lastUsedFrame = 0;
  };

  TransientTexturePool() = default;

  TransientTexturePool(const TransientTexturePool&) = delete;

  TransientTexturePool&
  operator=(const TransientTexturePool&) = delete;

  /**
   * Index of a texture no other request took this frame; it stays valid until nextFrame().
   */
  NODISCARD uint32
  acquire(const RGTextureDesc& desc, TextureUsageFlags usage);

  NODISCARD FORCEINLINE Entry&
  getEntry(uint32 index)
  {
    return m_entries[index];
  }

  /**
   * Called once per frame, after every graph that uses the pool ran.
   */
  void
  nextFrame();

  /**
   * Releases every texture; the GPU frees them through the deferred deletion.
   */
  void
  clear();

  NODISCARD FORCEINLINE SIZE_T
  getSize() const
  {
    return m_entries.size();
  }

  static constexpr uint64 FRAMES_TO_KEEP = 8;

 private:
  Vector<Entry> m_entries;
  // Starts at 1, so a new entry (lastUsedFrame 0) is never taken for used this frame.
  uint64 m_frame = 1;
};

} // namespace chEngineSDK
