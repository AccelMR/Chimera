/************************************************************************/
/**
 * @file chModelAsset.cpp
 * @author AccelMR
 * @date 2025/07/14
 * @brief
 *  Asset that holds a Model.
 */
/************************************************************************/
#include "chModelAsset.h"

#include "chAlgorithm.h"
#include "chAssetManager.h"
#include "chFileStream.h"
#include "chLogger.h"
#include "chMaterialAsset.h"
#include "chModel.h"
#include "chModelSerializer.h"
#include "chSkeleton.h"
#include "chSkeletonAsset.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(ModelAssetLog, All);

/*
 */
bool
ModelAsset::serialize(SPtr<DataStream> stream)
{
  if (!stream || !m_model) {
    CH_LOG(ModelAssetLog, Error, "ModelAsset {0} has no model to save", m_metadata.name);
    return false;
  }
  ModelSerializer::write(*stream, *m_model);
  return true;
}

/*
 */
bool
ModelAsset::deserialize(SPtr<DataStream> stream)
{
  if (!stream) {
    return false;
  }

  m_model = ModelSerializer::read(*stream);
  if (!m_model) {
    CH_LOG(ModelAssetLog, Error, "ModelAsset {0} could not be read", m_metadata.name);
    return false;
  }

  // IAsset::load already loaded the references, so the materials are ready when they exist.
  AssetManager& assetManager = AssetManager::instance();
  const Vector<ModelMaterialSlot>& slots = m_model->getMaterialSlots();
  for (uint32 slot = 0; slot < slots.size(); ++slot) {
    const UUID& materialId = slots[slot].materialId;
    if (materialId.isNull()) {
      continue;
    }
    const SPtr<IAsset> asset = assetManager.getAsset(materialId);
    const SPtr<MaterialAsset> material = asset ? asset->as<MaterialAsset>() : nullptr;
    if (material) {
      m_model->setSlotMaterial(slot, material);
    }
    else {
      CH_LOG(ModelAssetLog, Warning, "Model {0} lost the material of slot {1}",
             m_metadata.name, slots[slot].name);
    }
  }

  if (m_model->hasSkeleton()) {
    const SPtr<IAsset> asset = assetManager.getAsset(m_model->getSkeletonId());
    const SPtr<SkeletonAsset> skeleton = asset ? asset->as<SkeletonAsset>() : nullptr;
    if (!skeleton || !skeleton->getSkeleton()) {
      CH_LOG(ModelAssetLog, Warning, "Model {0} lost its skeleton {1}", m_metadata.name,
             m_model->getSkeletonId());
      return true;
    }
    const uint32 boneCount = skeleton->getSkeleton()->getBoneCount();
    for (const ModelNode& node : m_model->getNodes()) {
      if (node.bone != ModelNode::NO_BONE && node.bone >= boneCount) {
        CH_LOG(ModelAssetLog, Error, "Model {0} names bones its skeleton lacks",
               m_metadata.name);
        return true;
      }
    }
    m_model->setSkeleton(m_model->getSkeletonId(), skeleton);
  }
  return true;
}

/*
 */
void
ModelAsset::clearAssetData()
{
  m_model.reset();
}

/*
 */
void
ModelAsset::collectReferences(Vector<UUID>& outReferences) const
{
  if (!m_model) {
    return;
  }
  for (const ModelMaterialSlot& slot : m_model->getMaterialSlots()) {
    if (!slot.materialId.isNull() && !Algorithm::contains(outReferences, slot.materialId)) {
      outReferences.push_back(slot.materialId);
    }
  }
  if (m_model->hasSkeleton() &&
      !Algorithm::contains(outReferences, m_model->getSkeletonId())) {
    outReferences.push_back(m_model->getSkeletonId());
  }
}

} // namespace chEngineSDK
