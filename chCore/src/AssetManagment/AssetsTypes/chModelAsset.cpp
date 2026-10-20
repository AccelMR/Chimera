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
}

} // namespace chEngineSDK
