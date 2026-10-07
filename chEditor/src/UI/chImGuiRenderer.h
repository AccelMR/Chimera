/************************************************************************/
/**
 * @file chImGuiRenderer.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 * Draws Dear ImGui with the engine graphics interfaces.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"
#include "chPipelineCache.h"
#include "chVertexLayout.h"

struct ImDrawData;
struct ImTextureData;

namespace chEngineSDK {

/**
 * ImGui renderer backend written on the engine graphics interfaces, so the editor UI works
 * with every graphics API and needs no ImGui code inside the graphics plugins. Textures are
 * read through the bindless heap: an ImTextureID is a bindless index plus one, because ImGui
 * reserves 0 as the invalid id.
 */
class ImGuiRenderer
{
 public:
  /**
   * Needs a current ImGui context, which must outlive this renderer.
   */
  ImGuiRenderer();
  ~ImGuiRenderer();

  ImGuiRenderer(const ImGuiRenderer&) = delete;
  ImGuiRenderer&
  operator=(const ImGuiRenderer&) = delete;

  /**
   * Draws the UI over the target, which must be in the RenderTarget state. Begins and ends
   * its own rendering and keeps what is already in the target.
   */
  void
  render(ICommandList& commandList,
         ImDrawData& drawData,
         const ITextureView& target,
         Format targetFormat,
         uint32 targetWidth,
         uint32 targetHeight);

  /**
   * ImTextureID that shows the texture with this bindless index; 0 (ImGui's invalid id)
   * when the texture cannot be sampled.
   */
  NODISCARD static FORCEINLINE uint64
  getTextureId(uint32 bindlessIndex)
  {
    return bindlessIndex == GraphicsLimits::INVALID_BINDLESS_INDEX
               ? 0
               : static_cast<uint64>(bindlessIndex) + 1;
  }

 private:
  struct FrameBuffers
  {
    SPtr<IBuffer> vertexBuffer;
    SPtr<IBuffer> indexBuffer;
  };

  void
  updateTexture(ImTextureData& texture);

  void
  uploadGeometry(ImDrawData& drawData, FrameBuffers& frameBuffers);

  void
  setupRenderState(ICommandList& commandList,
                   const ImDrawData& drawData,
                   const FrameBuffers& frameBuffers,
                   uint32 targetWidth,
                   uint32 targetHeight);

  NODISCARD const IPipeline&
  getPipeline(Format targetFormat);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  VertexLayout m_vertexLayout;
  SPtr<ISampler> m_sampler;

  PipelineCache m_pipelineCache;
  // Owned by m_pipelineCache; looked up again only when the target format changes.
  const IPipeline* m_pipeline = nullptr;
  Format m_pipelineFormat = Format::Unknown;

  // One copy per frame in flight, because the CPU writes them while the GPU may still read
  // the previous frame's. They only grow; the old buffer goes through the deferred deletion.
  Array<FrameBuffers, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_frameBuffers;

  // Textures ImGui asked for (the font atlas), by ImTextureData::UniqueID.
  UnorderedMap<int32, SPtr<ITexture>> m_textures;
};

} // namespace chEngineSDK
