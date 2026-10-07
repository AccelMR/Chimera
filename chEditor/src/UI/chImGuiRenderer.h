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
struct ImGuiViewport;
struct ImVec2;

namespace chEngineSDK {

/**
 * ImGui renderer backend written on the engine graphics interfaces, so the editor UI works
 * with every graphics API and needs no ImGui code inside the graphics plugins. Textures are
 * read through the bindless heap: an ImTextureID is a bindless index plus one, because ImGui
 * reserves 0 as the invalid id. ImGui windows dragged outside the main one get a swap chain
 * each, drawn in the same frame and presented after the main window.
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
   * Draws the main window UI inside a rendering already begun on a target of that format
   * and size (a render graph pass that loads the target).
   */
  void
  render(ICommandList& commandList,
         ImDrawData& drawData,
         Format targetFormat,
         uint32 targetWidth,
         uint32 targetHeight);

  /**
   * Acquires and draws the swap chain of every ImGui window outside the main one. Called
   * between IGraphicsAPI::beginFrame and endFrame, after ImGui::UpdatePlatformWindows.
   */
  void
  renderFloatingWindows(ICommandList& commandList);

  /**
   * Presents what renderFloatingWindows drew; called after IGraphicsAPI::endFrame.
   */
  void
  presentFloatingWindows();

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
    // Bytes already written in the ImGui frame imguiFrame; every window of a frame writes
    // after the previous one.
    uint32 vertexCursor = 0;
    uint32 indexCursor = 0;
    int32 imguiFrame = -1;
  };

  struct GeometryRange
  {
    uint64 vertexOffset = 0;
    uint64 indexOffset = 0;
  };

  struct WindowData
  {
    SPtr<ISwapChain> swapChain;
    bool acquired = false;
    bool needsResize = false;
  };

  static void
  createWindow(ImGuiViewport* viewport);

  static void
  destroyWindow(ImGuiViewport* viewport);

  static void
  setWindowSize(ImGuiViewport* viewport, ImVec2 size);

  /**
   * Creates and updates the textures ImGui asked for, also when nothing is drawn, as
   * ImGui expects.
   */
  void
  updateTextures(ImDrawData& drawData);

  /**
   * Records the draws inside a rendering that is already begun.
   */
  void
  drawGeometry(ICommandList& commandList,
               ImDrawData& drawData,
               Format targetFormat,
               uint32 targetWidth,
               uint32 targetHeight);

  void
  updateTexture(ImTextureData& texture);

  NODISCARD GeometryRange
  uploadGeometry(const ImDrawData& drawData, FrameBuffers& frameBuffers);

  void
  setupRenderState(ICommandList& commandList,
                   const ImDrawData& drawData,
                   const FrameBuffers& frameBuffers,
                   const GeometryRange& geometry,
                   const IPipeline& pipeline,
                   uint32 targetWidth,
                   uint32 targetHeight);

  NODISCARD const IPipeline&
  getPipeline(Format targetFormat);

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  VertexLayout m_vertexLayout;
  SPtr<ISampler> m_sampler;

  PipelineCache m_pipelineCache;
  // Owned by m_pipelineCache, one per target format, so a frame never builds a description.
  Vector<Pair<Format, const IPipeline*>> m_pipelines;

  // One copy per frame in flight, because the CPU writes them while the GPU may still read
  // the previous frame's. They only grow; the old buffer goes through the deferred deletion.
  Array<FrameBuffers, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_frameBuffers;

  // Textures ImGui asked for (the font atlas), by ImTextureData::UniqueID.
  UnorderedMap<int32, SPtr<ITexture>> m_textures;

  // Windows outside the main one, by ImGuiViewport::ID. Nodes do not move, so each
  // viewport keeps a pointer to its entry.
  UnorderedMap<uint32, WindowData> m_windows;
};

} // namespace chEngineSDK
