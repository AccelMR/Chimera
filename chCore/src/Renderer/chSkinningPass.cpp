/************************************************************************/
/**
 * @file chSkinningPass.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief Moves the vertices of skinned meshes by their bones on the GPU.
 */
/************************************************************************/
#include "chSkinningPass.h"

#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chMath.h"
#include "chMesh.h"
#include "chRenderGraph.h"
#include "chVertexLayout.h"

namespace chEngineSDK {
namespace {
// Must match PushConstants in skinning.hlsl.
struct SkinningPushConstants
{
  uint32 sourceIndex;
  uint32 skinIndex;
  uint32 matrixIndex;
  uint32 firstMatrix;
  uint32 outputIndex;
  uint32 vertexCount;
};

// Threads per group in skinning.hlsl.
constexpr uint32 kGroupSize = 64;
constexpr uint32 kInitialMatrixCapacity = 256;
} // namespace

/*
 */
SkinningPass::SkinningPass()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_pipeline = graphicsAPI.createComputePipeline(
      {.computeShader = graphicsAPI.loadShader(ShaderStage::Compute, "skinning")});
}

/*
 */
SkinningPass::~SkinningPass() = default;

/*
 */
void
SkinningPass::addPass(RenderGraph& graph, Span<RenderItem> items)
{
  ++m_frame;
  m_jobs.clear();
  m_matrices.clear();

  for (RenderItem& item : items) {
    if (!item.skinMatrices) {
      continue;
    }
    SkinnedVertices* vertices = findOrCreateVertices(item);
    if (!vertices) {
      continue;
    }
    vertices->lastUsedFrame = m_frame;
    item.skinnedVertices = vertices->buffer.get();
    if (vertices->revision == item.skinRevision) {
      continue;
    }
    vertices->revision = item.skinRevision;

    const Mesh& mesh = *item.mesh;
    const SIZE_T boneCount = mesh.getSkin().boneNodes.size();
    m_jobs.push_back({.source = mesh.getVertexBuffer(),
                      .skin = mesh.getSkinBuffer(),
                      .output = vertices->buffer.get(),
                      .firstMatrix = static_cast<uint32>(m_matrices.size()),
                      .vertexCount = mesh.getVertexCount()});
    m_matrices.insert(m_matrices.end(), item.skinMatrices, item.skinMatrices + boneCount);
  }
  dropUnused();

  if (m_jobs.empty()) {
    return;
  }
  const uint32 matrixIndex = uploadMatrices();
  graph.addPass("Skinning")
      .setSideEffect()
      .setExecute([this, matrixIndex](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        // The draws of earlier frames may still read the buffers written here.
        commandList.memoryBarrier(ResourceState::VertexInput, ResourceState::UnorderedAccess);
        commandList.bindPipeline(*m_pipeline);
        for (const SkinJob& job : m_jobs) {
          const SkinningPushConstants pushConstants{
              .sourceIndex = job.source->getBindlessIndex(),
              .skinIndex = job.skin->getBindlessIndex(),
              .matrixIndex = matrixIndex,
              .firstMatrix = job.firstMatrix,
              .outputIndex = job.output->getBindlessIndex(),
              .vertexCount = job.vertexCount};
          commandList.pushConstants(&pushConstants, sizeof(pushConstants));
          commandList.dispatch((job.vertexCount + kGroupSize - 1) / kGroupSize);
        }
        commandList.memoryBarrier(ResourceState::UnorderedAccess, ResourceState::VertexInput);
      });
}

/*
 */
SkinningPass::SkinnedVertices*
SkinningPass::findOrCreateVertices(const RenderItem& item)
{
  const Mesh& mesh = *item.mesh;
  const MeshSkin& skin = mesh.getSkin();
  // The shader reads VertexStatic, and a skin must cover every vertex.
  if (mesh.getVertexLayout().getVertexSize() != sizeof(VertexStatic) ||
      skin.vertices.size() != mesh.getVertexCount() || !mesh.getVertexBuffer() ||
      !mesh.getSkinBuffer()) {
    return nullptr;
  }

  SkinnedVertices& vertices = m_vertices[item.sceneSlot];
  if (vertices.mesh == &mesh && vertices.buffer) {
    return &vertices;
  }
  vertices.mesh = &mesh;
  vertices.revision = 0;
  vertices.buffer = IGraphicsAPI::instance().createBuffer(
      {.size = static_cast<uint32>(mesh.getVertexDataSize()),
       .usage = BufferUsage::VertexBuffer | BufferUsage::StorageBuffer,
       .memoryUsage = MemoryUsage::GpuOnly});
  return vertices.buffer ? &vertices : nullptr;
}

/*
 */
uint32
SkinningPass::uploadMatrices()
{
  const uint32 frameIndex = IGraphicsAPI::instance().getFrameIndex();
  const uint32 count = static_cast<uint32>(m_matrices.size());
  uint32& capacity = m_matrixCapacities[frameIndex];
  if (count > capacity) {
    capacity = Math::max(count, Math::max(capacity * 2, kInitialMatrixCapacity));
    m_matrixBuffers[frameIndex] = IGraphicsAPI::instance().createBuffer(
        {.size = static_cast<uint32>(capacity * sizeof(Matrix4)),
         .usage = BufferUsage::StorageBuffer,
         .memoryUsage = MemoryUsage::CpuToGpu});
  }
  IBuffer& buffer = *m_matrixBuffers[frameIndex];
  buffer.update(m_matrices.data(), m_matrices.size() * sizeof(Matrix4));
  return buffer.getBindlessIndex();
}

/*
 */
void
SkinningPass::dropUnused()
{
  // Buffers go through the deferred deletion, so frames in flight can still draw them.
  for (auto it = m_vertices.begin(); it != m_vertices.end();) {
    if (m_frame - it->second.lastUsedFrame > FRAMES_TO_KEEP) {
      it = m_vertices.erase(it);
    }
    else {
      ++it;
    }
  }
}

} // namespace chEngineSDK
