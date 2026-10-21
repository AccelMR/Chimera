/************************************************************************/
/**
 * @file chMaterialPipelines.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief The pipelines one pass needs for every kind of material, and the draws that use
 *  them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"
#include "chICommandList.h"
#include "chMaterial.h"
#include "chMesh.h"
#include "chRenderItem.h"

namespace chEngineSDK {
class PipelineCache;

/**
 * Exists so every pass that draws items with their materials picks the pipeline the same
 * way: masked materials need a fragment shader that discards, which opaque ones must not
 * use because it turns off the early depth test, and two-sided materials need culling off.
 * It keeps the four versions of one description and draws the items grouped by them, so
 * each is bound at most once per pass.
 */
class CH_CORE_EXPORT MaterialPipelines
{
 public:
  /**
   * desc holds the fragment shader of opaque materials; maskedFragmentShader replaces it
   * for masked ones. Both may be the same shader (lines do not cut).
   */
  void
  build(PipelineCache& cache,
        GraphicsPipelineDesc desc,
        const SPtr<IShader>& maskedFragmentShader);

  NODISCARD FORCEINLINE bool
  isBuilt() const noexcept { return m_pipelines[0] != nullptr; }

  /**
   * Calls pushItemConstants(item, material) before each draw; it must push what the
   * shaders read for that item. Items without a material use defaultMaterial. Opaque
   * materials go first, so they fill the depth before the masked ones are tested.
   */
  template<typename PushItemConstants>
  void
  recordDraws(ICommandList& commandList,
              Span<const RenderItem* const> items,
              const Material& defaultMaterial,
              PushItemConstants&& pushItemConstants) const
  {
    CH_ASSERT(isBuilt());
    for (const uint32 variant : DRAW_ORDER) {
      bool bBound = false;
      for (const RenderItem* item : items) {
        const Material& material = item->material ? *item->material : defaultMaterial;
        if (getVariant(material) != variant) {
          continue;
        }
        // A skinned item draws the vertices the skinning wrote, or nothing without them:
        // the stored ones are in another space.
        const IBuffer* vertexBuffer =
            item->skinMatrices ? item->skinnedVertices : item->mesh->getVertexBuffer();
        const IBuffer* indexBuffer = item->mesh->getIndexBuffer();
        if (!vertexBuffer || !indexBuffer) {
          continue;
        }

        if (!bBound) {
          commandList.bindPipeline(*m_pipelines[variant]);
          bBound = true;
        }
        pushItemConstants(*item, material);
        commandList.bindVertexBuffer(*vertexBuffer);
        commandList.bindIndexBuffer(*indexBuffer, item->mesh->getIndexType());
        commandList.drawIndexed(item->mesh->getIndexCount());
      }
    }
  }

 private:
  NODISCARD static FORCEINLINE uint32
  getVariant(const Material& material) noexcept
  {
    return (material.getAlphaMode() == MaterialAlphaMode::Mask ? MASKED_BIT : 0u) |
           (material.isTwoSided() ? TWO_SIDED_BIT : 0u);
  }

  static constexpr uint32 MASKED_BIT = 1;
  static constexpr uint32 TWO_SIDED_BIT = 2;
  static constexpr uint32 VARIANT_COUNT = 4;
  static constexpr uint32 DRAW_ORDER[VARIANT_COUNT] = {0, TWO_SIDED_BIT, MASKED_BIT,
                                                       MASKED_BIT | TWO_SIDED_BIT};

  // Owned by the PipelineCache given to build.
  const IPipeline* m_pipelines[VARIANT_COUNT] = {};
};

} // namespace chEngineSDK
