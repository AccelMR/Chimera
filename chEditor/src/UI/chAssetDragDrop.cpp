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
#include "chMaterialAsset.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chScene.h"
#include "chTextureAsset.h"

#include "imgui.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(AssetDragDropLog, All);

namespace {
// ImGui copies the payload bytes, so the UUID travels by value and the asset is looked up
// again on drop, in case it was deleted during the drag.
static_assert(std::is_trivially_copyable_v<UUID>);

constexpr const ANSICHAR* kModelPayload = "CH_MODEL";
constexpr const ANSICHAR* kTexturePayload = "CH_TEXTURE";
constexpr const ANSICHAR* kMaterialPayload = "CH_MATERIAL";

/*
 * Null for asset types that cannot be dropped anywhere.
 */
const ANSICHAR*
getPayloadName(const IAsset& asset)
{
  if (asset.isTypeOf<ModelAsset>()) {
    return kModelPayload;
  }
  if (asset.isTypeOf<TextureAsset>()) {
    return kTexturePayload;
  }
  if (asset.isTypeOf<MaterialAsset>()) {
    return kMaterialPayload;
  }
  return nullptr;
}

/*
 */
template <typename AssetType>
SPtr<AssetType>
acceptAsset(const ANSICHAR* payloadName)
{
  const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadName);
  if (!payload || payload->DataSize != static_cast<int32>(sizeof(UUID))) {
    return nullptr;
  }

  UUID uuid;
  std::memcpy(&uuid, payload->Data, sizeof(UUID));

  AssetManager& assetManager = AssetManager::instance();
  const SPtr<IAsset> asset = assetManager.getAsset(uuid);
  if (!asset || !asset->isTypeOf<AssetType>()) {
    CH_LOG_WARNING(AssetDragDropLog, "The dropped asset no longer exists.");
    return nullptr;
  }
  if (!assetManager.syncLoadAsset(asset)) {
    CH_LOG_ERROR(AssetDragDropLog, "Failed to load asset: {0}", asset->getName());
    return nullptr;
  }
  return std::static_pointer_cast<AssetType>(asset);
}
} // namespace

/*
 */
void
AssetDragDrop::source(const IAsset& asset)
{
  const ANSICHAR* payloadName = getPayloadName(asset);
  if (!payloadName) {
    return;
  }

  if (ImGui::BeginDragDropSource()) {
    const UUID& uuid = asset.getUUID();
    ImGui::SetDragDropPayload(payloadName, &uuid, sizeof(UUID));
    ImGui::TextUnformatted(asset.getName());
    ImGui::EndDragDropSource();
  }
}

/*
 */
SPtr<ModelAsset>
AssetDragDrop::acceptModel()
{
  return acceptAsset<ModelAsset>(kModelPayload);
}

/*
 */
SPtr<TextureAsset>
AssetDragDrop::acceptTexture()
{
  return acceptAsset<TextureAsset>(kTexturePayload);
}

/*
 */
SPtr<MaterialAsset>
AssetDragDrop::acceptMaterial()
{
  return acceptAsset<MaterialAsset>(kMaterialPayload);
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
