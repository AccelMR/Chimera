/************************************************************************/
/**
 * @file chImGuiRenderer.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 * Draws Dear ImGui with the engine graphics interfaces.
 */
/************************************************************************/
#include "chImGuiRenderer.h"

#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chISampler.h"
#include "chIShader.h"
#include "chISwapChain.h"
#include "chITexture.h"
#include "chLogger.h"
#include "chMath.h"

#include "imgui.h"

#if USING(CH_DISPLAY_SDL3)
#include <SDL3/SDL_video.h>
#endif // USING(CH_DISPLAY_SDL3)

CH_LOG_DECLARE_STATIC(ImGuiRendererLog, All);

namespace chEngineSDK {

namespace {
// Must match PushConstants in imgui.hlsl.
struct ImGuiPushConstants
{
  float scaleX;
  float scaleY;
  float translateX;
  float translateY;
  uint32 textureIndex;
  uint32 samplerIndex;
};
static_assert(sizeof(ImGuiPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

constexpr IndexType kIndexType =
    sizeof(ImDrawIdx) == 2 ? IndexType::UInt16 : IndexType::UInt32;

// First size of the geometry buffers, enough for a typical editor frame, so they rarely
// have to grow.
constexpr uint32 kInitialVertexBufferSize = 512 * 1024;
constexpr uint32 kInitialIndexBufferSize = 128 * 1024;

/*
 * Makes sure the buffer holds size bytes and returns true when it had to be replaced. A new
 * buffer starts empty.
 */
bool
reserveBuffer(SPtr<IBuffer>& buffer, SIZE_T size, uint32 initialSize, BufferUsageFlags usage)
{
  const SIZE_T capacity = buffer ? buffer->getSize() : 0;
  if (size <= capacity) {
    return false;
  }

  const SIZE_T newSize = Math::max(size, Math::max(capacity * 2, SIZE_T(initialSize)));
  buffer = IGraphicsAPI::instance().createBuffer({.size = static_cast<uint32>(newSize),
                                                  .usage = usage,
                                                  .memoryUsage = MemoryUsage::CpuToGpu});
  return true;
}

NODISCARD ImGuiRenderer&
getRenderer()
{
  return *static_cast<ImGuiRenderer*>(ImGui::GetIO().BackendRendererUserData);
}

NODISCARD PlatformDisplay
getViewportWindow(const ImGuiViewport& viewport)
{
#if USING(CH_DISPLAY_SDL3)
  // The SDL3 platform backend keeps the SDL window id in PlatformHandle.
  return SDL_GetWindowFromID(
      static_cast<SDL_WindowID>(reinterpret_cast<SIZE_T>(viewport.PlatformHandle)));
#else
  return static_cast<PlatformDisplay>(viewport.PlatformHandle);
#endif // USING(CH_DISPLAY_SDL3)
}

NODISCARD uint32
toPixels(float size, float framebufferScale)
{
  return static_cast<uint32>(size * framebufferScale);
}
} // namespace

/*
 */
ImGuiRenderer::ImGuiRenderer()
{
  ImGuiIO& io = ImGui::GetIO();
  CH_ASSERT(io.BackendRendererUserData == nullptr && "An ImGui renderer is already set.");
  io.BackendRendererUserData = this;
  io.BackendRendererName = "Chimera";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

  // ImGui only calls them when ImGuiConfigFlags_ViewportsEnable is set.
  ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
  platformIO.Renderer_CreateWindow = &ImGuiRenderer::createWindow;
  platformIO.Renderer_DestroyWindow = &ImGuiRenderer::destroyWindow;
  platformIO.Renderer_SetWindowSize = &ImGuiRenderer::setWindowSize;

  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "imgui");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "imgui");

  m_vertexLayout.addAttribute(VertexAttributeType::Position, VertexFormat::Float2,
                              offsetof(ImDrawVert, pos));
  m_vertexLayout.addAttribute(VertexAttributeType::TexCoord0, VertexFormat::Float2,
                              offsetof(ImDrawVert, uv));
  m_vertexLayout.addAttribute(VertexAttributeType::Color, VertexFormat::UByte4Normalized,
                              offsetof(ImDrawVert, col));
  CH_ASSERT(m_vertexLayout.getStride() == sizeof(ImDrawVert));

  m_sampler = graphicsAPI.createSampler(
      {.addressModeU = SamplerAddressMode::ClampToEdge,
       .addressModeV = SamplerAddressMode::ClampToEdge,
       .addressModeW = SamplerAddressMode::ClampToEdge});
}

/*
 */
ImGuiRenderer::~ImGuiRenderer()
{
  // Closes the floating windows while their swap chains can still be released here, and
  // before the platform backend that made their windows shuts down.
  ImGui::DestroyPlatformWindows();
  CH_ASSERT(m_windows.empty());

  // Textures still used by another context are left to it.
  for (ImTextureData* texture : ImGui::GetPlatformIO().Textures) {
    if (texture->RefCount == 1) {
      texture->SetTexID(ImTextureID_Invalid);
      texture->SetStatus(ImTextureStatus_Destroyed);
    }
  }
  m_textures.clear();

  ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
  platformIO.Renderer_CreateWindow = nullptr;
  platformIO.Renderer_DestroyWindow = nullptr;
  platformIO.Renderer_SetWindowSize = nullptr;

  ImGuiIO& io = ImGui::GetIO();
  io.BackendRendererUserData = nullptr;
  io.BackendRendererName = nullptr;
  io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset |
                       ImGuiBackendFlags_RendererHasTextures |
                       ImGuiBackendFlags_RendererHasViewports);
}

/*
 */
void
ImGuiRenderer::render(ICommandList& commandList,
                      ImDrawData& drawData,
                      const ITextureView& target,
                      Format targetFormat,
                      uint32 targetWidth,
                      uint32 targetHeight)
{
  recordDrawData(commandList, drawData, target, targetFormat, targetWidth, targetHeight,
                 LoadOp::Load);
}

/*
 */
void
ImGuiRenderer::renderFloatingWindows(ICommandList& commandList)
{
  const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

  // The first viewport is the main window, which the application draws.
  for (int32 i = 1; i < platformIO.Viewports.Size; ++i) {
    ImGuiViewport& viewport = *platformIO.Viewports[i];
    WindowData* window = static_cast<WindowData*>(viewport.RendererUserData);
    if (window == nullptr || viewport.DrawData == nullptr ||
        (viewport.Flags & ImGuiViewportFlags_IsMinimized)) {
      continue;
    }

    ISwapChain& swapChain = *window->swapChain;
    const SwapChainStatus status = swapChain.acquireNextImage();
    if (status == SwapChainStatus::OutOfDate || status == SwapChainStatus::Failed) {
      window->needsResize = status == SwapChainStatus::OutOfDate;
      continue;
    }
    window->acquired = true;
    window->needsResize = status == SwapChainStatus::Suboptimal;

    const ITexture& image = swapChain.getCurrentTexture();
    const Array<TextureBarrier, 1> toRendering = {
        TextureBarrier{.texture = &image,
                       .before = ResourceState::Undefined,
                       .after = ResourceState::RenderTarget}};
    commandList.barrier(toRendering);

    // A window ImGui fills completely does not need the clear.
    const LoadOp loadOp = (viewport.Flags & ImGuiViewportFlags_NoRendererClear)
                              ? LoadOp::DontCare
                              : LoadOp::Clear;
    recordDrawData(commandList, *viewport.DrawData, swapChain.getCurrentTextureView(),
                   swapChain.getFormat(), swapChain.getWidth(), swapChain.getHeight(),
                   loadOp);

    const Array<TextureBarrier, 1> toPresent = {
        TextureBarrier{.texture = &image,
                       .before = ResourceState::RenderTarget,
                       .after = ResourceState::Present}};
    commandList.barrier(toPresent);
  }
}

/*
 */
void
ImGuiRenderer::presentFloatingWindows()
{
  const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
  for (int32 i = 1; i < platformIO.Viewports.Size; ++i) {
    ImGuiViewport& viewport = *platformIO.Viewports[i];
    WindowData* window = static_cast<WindowData*>(viewport.RendererUserData);
    if (window == nullptr) {
      continue;
    }

    if (window->acquired) {
      window->acquired = false;
      const SwapChainStatus status = window->swapChain->present();
      if (status == SwapChainStatus::Suboptimal || status == SwapChainStatus::OutOfDate) {
        window->needsResize = true;
      }
    }

    // After the present, so the image of this frame was not drawn on a destroyed swap chain.
    if (window->needsResize) {
      window->needsResize = false;
      window->swapChain->resize(toPixels(viewport.Size.x, viewport.FramebufferScale.x),
                                toPixels(viewport.Size.y, viewport.FramebufferScale.y));
    }
  }
}

/*
 */
void
ImGuiRenderer::createWindow(ImGuiViewport* viewport)
{
  WindowData& window = getRenderer().m_windows[viewport->ID];
  // Without vsync, like the main window.
  window.swapChain = IGraphicsAPI::instance().createSwapChain(
      {.window = getViewportWindow(*viewport),
       .width = toPixels(viewport->Size.x, viewport->FramebufferScale.x),
       .height = toPixels(viewport->Size.y, viewport->FramebufferScale.y),
       .vsync = false,
       .debugName = "ImGui Window SwapChain"});
  viewport->RendererUserData = &window;
}

/*
 */
void
ImGuiRenderer::destroyWindow(ImGuiViewport* viewport)
{
  // The main viewport has no data: the application owns its swap chain. Releasing the swap
  // chain waits for the GPU, which is fine because windows close rarely.
  if (viewport->RendererUserData != nullptr) {
    getRenderer().m_windows.erase(viewport->ID);
    viewport->RendererUserData = nullptr;
  }
}

/*
 */
void
ImGuiRenderer::setWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
  WindowData* window = static_cast<WindowData*>(viewport->RendererUserData);
  if (window != nullptr) {
    window->swapChain->resize(toPixels(size.x, viewport->FramebufferScale.x),
                              toPixels(size.y, viewport->FramebufferScale.y));
  }
}

/*
 */
void
ImGuiRenderer::recordDrawData(ICommandList& commandList,
                              ImDrawData& drawData,
                              const ITextureView& target,
                              Format targetFormat,
                              uint32 targetWidth,
                              uint32 targetHeight,
                              LoadOp loadOp)
{
  // Textures are created and updated even when nothing is drawn, as ImGui expects.
  if (drawData.Textures != nullptr) {
    for (ImTextureData* texture : *drawData.Textures) {
      if (texture->Status != ImTextureStatus_OK) {
        updateTexture(*texture);
      }
    }
  }

  if (targetWidth == 0 || targetHeight == 0) {
    return;
  }

  // Begun even without geometry, so a cleared target is still cleared.
  RenderingDesc renderingDesc{.colorAttachmentCount = 1,
                              .width = targetWidth,
                              .height = targetHeight};
  renderingDesc.colorAttachments[0] = {.view = &target, .loadOp = loadOp};
  commandList.beginRendering(renderingDesc);

  if (drawData.TotalVtxCount == 0) {
    commandList.endRendering();
    return;
  }

  FrameBuffers& frameBuffers = m_frameBuffers[IGraphicsAPI::instance().getFrameIndex()];
  const GeometryRange geometry = uploadGeometry(drawData, frameBuffers);
  const IPipeline& pipeline = getPipeline(targetFormat);
  setupRenderState(commandList, drawData, frameBuffers, geometry, pipeline, targetWidth,
                   targetHeight);

  // Clip rectangles come in ImGui coordinates; the scissor is in target pixels.
  const ImVec2 clipOffset = drawData.DisplayPos;
  const ImVec2 clipScale = drawData.FramebufferScale;
  const float maxX = static_cast<float>(targetWidth);
  const float maxY = static_cast<float>(targetHeight);

  uint32 boundTextureIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
  uint32 globalIndexOffset = 0;
  int32 globalVertexOffset = 0;
  for (const ImDrawList* drawList : drawData.CmdLists) {
    for (const ImDrawCmd& drawCommand : drawList->CmdBuffer) {
      if (drawCommand.UserCallback != nullptr) {
        if (drawCommand.UserCallback == ImDrawCallback_ResetRenderState) {
          setupRenderState(commandList, drawData, frameBuffers, geometry, pipeline,
                           targetWidth, targetHeight);
          boundTextureIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
        }
        else {
          drawCommand.UserCallback(drawList, &drawCommand);
        }
        continue;
      }

      const float clipMinX = Math::max((drawCommand.ClipRect.x - clipOffset.x) * clipScale.x,
                                       0.0f);
      const float clipMinY = Math::max((drawCommand.ClipRect.y - clipOffset.y) * clipScale.y,
                                       0.0f);
      const float clipMaxX = Math::min((drawCommand.ClipRect.z - clipOffset.x) * clipScale.x,
                                       maxX);
      const float clipMaxY = Math::min((drawCommand.ClipRect.w - clipOffset.y) * clipScale.y,
                                       maxY);
      if (clipMaxX <= clipMinX || clipMaxY <= clipMinY) {
        continue;
      }
      commandList.setScissor(static_cast<uint32>(clipMinX),
                             static_cast<uint32>(clipMinY),
                             static_cast<uint32>(clipMaxX - clipMinX),
                             static_cast<uint32>(clipMaxY - clipMinY));

      const ImTextureID textureId = drawCommand.GetTexID();
      CH_ASSERT(textureId != ImTextureID_Invalid);
      const uint32 textureIndex = static_cast<uint32>(textureId - 1);
      if (textureIndex != boundTextureIndex) {
        commandList.pushConstants(&textureIndex, sizeof(textureIndex),
                                  offsetof(ImGuiPushConstants, textureIndex));
        boundTextureIndex = textureIndex;
      }

      commandList.drawIndexed(drawCommand.ElemCount,
                              1,
                              drawCommand.IdxOffset + globalIndexOffset,
                              static_cast<int32>(drawCommand.VtxOffset) + globalVertexOffset,
                              0);
    }
    globalIndexOffset += static_cast<uint32>(drawList->IdxBuffer.Size);
    globalVertexOffset += drawList->VtxBuffer.Size;
  }

  commandList.endRendering();
}

/*
 */
void
ImGuiRenderer::updateTexture(ImTextureData& texture)
{
  // GPU objects are freed by the deferred deletion, so a texture can be replaced or
  // destroyed at once even if a frame in flight still samples it.
  if (texture.Status == ImTextureStatus_WantCreate) {
    CH_ASSERT(texture.Format == ImTextureFormat_RGBA32);
    SPtr<ITexture> gpuTexture = IGraphicsAPI::instance().createTexture(
        {.format = Format::R8G8B8A8_UNORM,
         .width = static_cast<uint32>(texture.Width),
         .height = static_cast<uint32>(texture.Height),
         .initialData = texture.GetPixels(),
         .initialDataSize = static_cast<SIZE_T>(texture.GetSizeInBytes())});
    texture.SetTexID(getTextureId(gpuTexture->getBindlessIndex()));
    m_textures[texture.UniqueID] = std::move(gpuTexture);
    texture.SetStatus(ImTextureStatus_OK);
  }
  else if (texture.Status == ImTextureStatus_WantUpdates) {
    // The whole texture is uploaded again instead of the changed rectangles. It only
    // happens when ImGui adds glyphs to the atlas, and keeps the graphics interface small.
    const auto it = m_textures.find(texture.UniqueID);
    CH_ASSERT(it != m_textures.end());
    it->second->uploadData(texture.GetPixels(), static_cast<SIZE_T>(texture.GetSizeInBytes()));
    texture.SetStatus(ImTextureStatus_OK);
  }
  else if (texture.Status == ImTextureStatus_WantDestroy) {
    m_textures.erase(texture.UniqueID);
    texture.SetTexID(ImTextureID_Invalid);
    texture.SetStatus(ImTextureStatus_Destroyed);
  }
}

/*
 */
ImGuiRenderer::GeometryRange
ImGuiRenderer::uploadGeometry(const ImDrawData& drawData, FrameBuffers& frameBuffers)
{
  // The first window of an ImGui frame writes from the start again: the frames in flight
  // that may still read this slot finished before IGraphicsAPI::beginFrame returned.
  const int32 imguiFrame = ImGui::GetFrameCount();
  if (frameBuffers.imguiFrame != imguiFrame) {
    frameBuffers.imguiFrame = imguiFrame;
    frameBuffers.vertexCursor = 0;
    frameBuffers.indexCursor = 0;
  }

  // A replaced buffer starts empty. Windows drawn before in this frame keep the old one,
  // which the deferred deletion frees after them.
  const SIZE_T vertexBytes = static_cast<SIZE_T>(drawData.TotalVtxCount) * sizeof(ImDrawVert);
  const SIZE_T indexBytes = static_cast<SIZE_T>(drawData.TotalIdxCount) * sizeof(ImDrawIdx);
  if (reserveBuffer(frameBuffers.vertexBuffer, frameBuffers.vertexCursor + vertexBytes,
                    kInitialVertexBufferSize, BufferUsage::VertexBuffer)) {
    frameBuffers.vertexCursor = 0;
  }
  if (reserveBuffer(frameBuffers.indexBuffer, frameBuffers.indexCursor + indexBytes,
                    kInitialIndexBufferSize, BufferUsage::IndexBuffer)) {
    frameBuffers.indexCursor = 0;
  }

  const GeometryRange geometry{.vertexOffset = frameBuffers.vertexCursor,
                               .indexOffset = frameBuffers.indexCursor};
  for (const ImDrawList* drawList : drawData.CmdLists) {
    const SIZE_T listVertexBytes =
        static_cast<SIZE_T>(drawList->VtxBuffer.Size) * sizeof(ImDrawVert);
    const SIZE_T listIndexBytes =
        static_cast<SIZE_T>(drawList->IdxBuffer.Size) * sizeof(ImDrawIdx);
    frameBuffers.vertexBuffer->update(drawList->VtxBuffer.Data, listVertexBytes,
                                      frameBuffers.vertexCursor);
    frameBuffers.indexBuffer->update(drawList->IdxBuffer.Data, listIndexBytes,
                                     frameBuffers.indexCursor);
    frameBuffers.vertexCursor += static_cast<uint32>(listVertexBytes);
    frameBuffers.indexCursor += static_cast<uint32>(listIndexBytes);
  }
  return geometry;
}

/*
 */
void
ImGuiRenderer::setupRenderState(ICommandList& commandList,
                                const ImDrawData& drawData,
                                const FrameBuffers& frameBuffers,
                                const GeometryRange& geometry,
                                const IPipeline& pipeline,
                                uint32 targetWidth,
                                uint32 targetHeight)
{
  commandList.bindPipeline(pipeline);
  commandList.bindVertexBuffer(*frameBuffers.vertexBuffer, 0, geometry.vertexOffset);
  commandList.bindIndexBuffer(*frameBuffers.indexBuffer, kIndexType, geometry.indexOffset);
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(targetWidth),
                          static_cast<float>(targetHeight));

  // Maps ImGui coordinates (pixels, Y down, from DisplayPos) to clip space, whose Y points
  // up in the engine.
  const float scaleX = 2.0f / drawData.DisplaySize.x;
  const float scaleY = -2.0f / drawData.DisplaySize.y;
  const ImGuiPushConstants pushConstants{
      .scaleX = scaleX,
      .scaleY = scaleY,
      .translateX = -1.0f - drawData.DisplayPos.x * scaleX,
      .translateY = 1.0f - drawData.DisplayPos.y * scaleY,
      .textureIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .samplerIndex = m_sampler->getBindlessIndex()};
  commandList.pushConstants(&pushConstants, sizeof(pushConstants));
}

/*
 */
const IPipeline&
ImGuiRenderer::getPipeline(Format targetFormat)
{
  for (const Pair<Format, const IPipeline*>& entry : m_pipelines) {
    if (entry.first == targetFormat) {
      return *entry.second;
    }
  }

  GraphicsPipelineDesc desc{.vertexShader = m_vertexShader,
                            .fragmentShader = m_fragmentShader,
                            .vertexLayout = m_vertexLayout,
                            .raster = {.cullMode = CullMode::None},
                            .depth = {.testEnable = false, .writeEnable = false},
                            .colorAttachmentCount = 1};
  desc.colorFormats[0] = targetFormat;
  desc.blendStates[0] = {.enable = true,
                         .srcColorFactor = BlendFactor::SrcAlpha,
                         .dstColorFactor = BlendFactor::OneMinusSrcAlpha,
                         .colorOp = BlendOp::Add,
                         .srcAlphaFactor = BlendFactor::One,
                         .dstAlphaFactor = BlendFactor::OneMinusSrcAlpha,
                         .alphaOp = BlendOp::Add};

  const IPipeline* pipeline = m_pipelineCache.getOrCreate(desc).get();
  m_pipelines.emplace_back(targetFormat, pipeline);
  return *pipeline;
}

} // namespace chEngineSDK
