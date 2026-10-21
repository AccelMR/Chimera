/************************************************************************/
/**
 * @file chSkinningPass.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief Moves the vertices of skinned meshes by their bones on the GPU.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"
#include "chMatrix4.h"
#include "chRenderItem.h"

namespace chEngineSDK {
class RenderGraph;

/**
 * Exists so every pass draws skinned meshes like any other mesh: before them, a compute
 * pass moves the vertices of each skinned item by its bones into a buffer of the same
 * layout, kept per item, and the frame's copy of the item points at it. An item is skinned
 * again only when its pose changed (RenderItem::skinRevision), also when the camera does
 * not see it, because it can still cast a shadow.
 */
class CH_CORE_EXPORT SkinningPass
{
 public:
  SkinningPass();
  ~SkinningPass();

  /**
   * Gives every skinned item its skinned vertices and adds the Skinning pass when one of
   * them changed. Must come before every pass that draws the items.
   */
  void
  addPass(RenderGraph& graph, Span<RenderItem> items);

  // Frames a buffer is kept after its item stops drawing, so an item that comes back soon
  // (a model hidden for a moment) is not made again.
  static constexpr uint64 FRAMES_TO_KEEP = 8;

 private:
  /**
   * The skinned vertices of one item.
   */
  struct SkinnedVertices
  {
    SPtr<IBuffer> buffer;
    const Mesh* mesh = nullptr;
    uint64 revision = 0;
    uint64 lastUsedFrame = 0;
  };

  /**
   * One dispatch of this frame's pass.
   */
  struct SkinJob
  {
    const IBuffer* source = nullptr;
    const IBuffer* skin = nullptr;
    const IBuffer* output = nullptr;
    uint32 firstMatrix = 0;
    uint32 vertexCount = 0;
  };

  /**
   * The buffer of the item's slot, made again when the slot now holds another mesh.
   */
  NODISCARD SkinnedVertices*
  findOrCreateVertices(const RenderItem& item);

  /**
   * Copies this frame's matrices into the buffer of the frame slot, which grows when it
   * is too small. Returns its bindless index.
   */
  NODISCARD uint32
  uploadMatrices();

  void
  dropUnused();

  SPtr<IPipeline> m_pipeline;
  // By RenderItem::sceneSlot.
  UnorderedMap<uint32, SkinnedVertices> m_vertices;
  // Rebuilt every frame and read when the pass runs; they keep their memory.
  Vector<SkinJob> m_jobs;
  Vector<Matrix4> m_matrices;
  // The CPU writes the matrices every frame, so each frame slot has its own buffer.
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_matrixBuffers;
  Array<uint32, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_matrixCapacities{};
  uint64 m_frame = 0;
};

} // namespace chEngineSDK
