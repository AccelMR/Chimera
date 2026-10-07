/************************************************************************/
/**
 * @file chAssetDragDrop.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @details
 *  Assets dragged from the Content Browser and dropped on other editor windows.
 */
/************************************************************************/
#include "chAssetDragDrop.h"

#include <cstring>
#include <type_traits>

#include "chAssetManager.h"
#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chLogger.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chScene.h"

#include "imgui.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(AssetDragDropLog, All);

namespace {
// ImGui copies the payload bytes, so the UUID travels by value and the asset is looked up
// again on drop, in case it was deleted during the drag.
static_assert(std::is_trivially_copyable_v<UUID>);

constexpr const ANSICHAR* kModelPayload = "CH_MODEL";
} // namespace

/*
 */
void
AssetDragDrop::source(const IAsset& asset)
{
  if (!asset.isTypeOf<ModelAsset>()) {
    return;
  }

  if (ImGui::BeginDragDropSource()) {
    const UUID& uuid = asset.getUUID();
    ImGui::SetDragDropPayload(kModelPayload, &uuid, sizeof(UUID));
    ImGui::TextUnformatted(asset.getName());
    ImGui::EndDragDropSource();
  }
}

/*
 */
SPtr<ModelAsset>
AssetDragDrop::acceptModel()
{
  const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kModelPayload);
  if (!payload || payload->DataSize != static_cast<int32>(sizeof(UUID))) {
    return nullptr;
  }

  UUID uuid;
  std::memcpy(&uuid, payload->Data, sizeof(UUID));

  AssetManager& assetManager = AssetManager::instance();
  const SPtr<IAsset> asset = assetManager.getAsset(uuid);
  if (!asset || !asset->isTypeOf<ModelAsset>()) {
    CH_LOG_WARNING(AssetDragDropLog, "The dropped model no longer exists.");
    return nullptr;
  }
  if (!assetManager.syncLoadAsset(asset)) {
    CH_LOG_ERROR(AssetDragDropLog, "Failed to load model: {0}", asset->getName());
    return nullptr;
  }
  return std::static_pointer_cast<ModelAsset>(asset);
}

/*
 */
SPtr<GameObject>
AssetDragDrop::createModelObject(Scene& scene, const ModelAsset& modelAsset,
                                 GameObject* parent)
{
  if (!modelAsset.getModel()) {
    CH_LOG_ERROR(AssetDragDropLog, "Model asset '{0}' has no model.", modelAsset.getName());
    return nullptr;
  }

  SPtr<GameObject> gameObject = scene.createGameObject(modelAsset.getName(), parent);
  gameObject->addComponent<ModelComponent>(modelAsset.getModel());
  EditorSelection::setSelectedGameObject(gameObject);
  return gameObject;
}

} // namespace chEngineSDK
