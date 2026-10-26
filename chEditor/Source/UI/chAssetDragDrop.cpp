/************************************************************************/
/**
 * @file chAssetDragDrop.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @details
 *  Assets dragged from the Content Browser and GameObjects dragged from the Scene Graph.
 */
/************************************************************************/
#include "chAssetDragDrop.h"
#include "chAnimationClipAsset.h"
#include "chAnimationGraphAsset.h"
#include "chScriptAsset.h"

#include <cstring>
#include <type_traits>

#include "chAnimatorComponent.h"
#include "chAssetManager.h"
#include "chEditorScene.h"
#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chLogger.h"
#include "chMaterialAsset.h"
#include "chModel.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chModelExpand.h"
#include "chGameObjectSerializer.h"
#include "chPrefabAsset.h"
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
constexpr const ANSICHAR* kPrefabPayload = "CH_PREFAB";
constexpr const ANSICHAR* kAnimationPayload = "CH_ANIMATION";
constexpr const ANSICHAR* kAnimationGraphPayload = "CH_ANIMATION_GRAPH";
constexpr const ANSICHAR* kScriptPayload = "CH_SCRIPT";
constexpr const ANSICHAR* kOtherAssetPayload = "CH_ASSET";
constexpr const ANSICHAR* kGameObjectPayload = "CH_GAMEOBJECT";
constexpr const ANSICHAR* kFolderPayload = "CH_FOLDER";

constexpr Array<const ANSICHAR*, 8> kAssetPayloads = {
    kModelPayload,     kTexturePayload,        kMaterialPayload, kPrefabPayload,
    kAnimationPayload, kAnimationGraphPayload, kScriptPayload,   kOtherAssetPayload};

/*
 * Targets ask for the dragged object every frame of a drag, so the last one found is kept
 * instead of walking the scene each time.
 */
WeakPtr<GameObject>&
getDraggedCache()
{
  static WeakPtr<GameObject> dragged;
  return dragged;
}

/*
 */
SPtr<GameObject>
findPayloadGameObject(const ImGuiPayload& payload)
{
  if (!payload.IsDataType(kGameObjectPayload) ||
      payload.DataSize != static_cast<int32>(sizeof(UUID))) {
    return nullptr;
  }
  UUID uuid;
  std::memcpy(&uuid, payload.Data, sizeof(UUID));

  WeakPtr<GameObject>& cache = getDraggedCache();
  SPtr<GameObject> gameObject = cache.lock();
  if (gameObject && gameObject->getUUID() == uuid && gameObject->getScene()) {
    return gameObject;
  }
  const SPtr<Scene>& scene = EditorScene::getScene();
  gameObject = scene ? scene->findGameObject(uuid) : nullptr;
  cache = gameObject;
  return gameObject;
}

/*
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
  if (asset.isTypeOf<PrefabAsset>()) {
    return kPrefabPayload;
  }
  if (asset.isTypeOf<AnimationClipAsset>()) {
    return kAnimationPayload;
  }
  if (asset.isTypeOf<AnimationGraphAsset>()) {
    return kAnimationGraphPayload;
  }
  if (asset.isTypeOf<ScriptAsset>()) {
    return kScriptPayload;
  }
  return kOtherAssetPayload;
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
AssetDragDrop::source(const IAsset& asset, uint32 selectedCount)
{
  const ANSICHAR* payloadName = getPayloadName(asset);
  if (ImGui::BeginDragDropSource()) {
    const UUID& uuid = asset.getUUID();
    ImGui::SetDragDropPayload(payloadName, &uuid, sizeof(UUID));
    if (selectedCount > 1) {
      ImGui::Text("%u assets", selectedCount);
    }
    else {
      ImGui::TextUnformatted(asset.getName());
    }
    ImGui::EndDragDropSource();
  }
}

/*
 */
SPtr<IAsset>
AssetDragDrop::acceptAnyAsset()
{
  const ImGuiPayload* dragged = ImGui::GetDragDropPayload();
  if (!dragged) {
    return nullptr;
  }
  for (const ANSICHAR* payloadName : kAssetPayloads) {
    if (!dragged->IsDataType(payloadName)) {
      continue;
    }
    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadName);
    if (!payload || payload->DataSize != static_cast<int32>(sizeof(UUID))) {
      return nullptr;
    }
    UUID uuid;
    std::memcpy(&uuid, payload->Data, sizeof(UUID));
    return AssetManager::instance().getAsset(uuid);
  }
  return nullptr;
}

/*
 */
void
AssetDragDrop::setFolderPayload(const String& folder)
{
  // ImGui copies the bytes.
  ImGui::SetDragDropPayload(kFolderPayload, folder.data(), folder.size());
}

/*
 */
bool
AssetDragDrop::acceptFolder(String& outFolder)
{
  const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kFolderPayload);
  if (!payload) {
    return false;
  }
  outFolder.assign(static_cast<const ANSICHAR*>(payload->Data),
                   static_cast<SIZE_T>(payload->DataSize));
  return true;
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
SPtr<PrefabAsset>
AssetDragDrop::acceptPrefab()
{
  return acceptAsset<PrefabAsset>(kPrefabPayload);
}

/*
 */
SPtr<AnimationClipAsset>
AssetDragDrop::acceptAnimation()
{
  return acceptAsset<AnimationClipAsset>(kAnimationPayload);
}

/*
 */
SPtr<AnimationGraphAsset>
AssetDragDrop::acceptAnimationGraph()
{
  return acceptAsset<AnimationGraphAsset>(kAnimationGraphPayload);
}

/*
 */
SPtr<ScriptAsset>
AssetDragDrop::acceptScript()
{
  return acceptAsset<ScriptAsset>(kScriptPayload);
}

/*
 */
void
AssetDragDrop::source(const GameObject& gameObject)
{
  if (ImGui::BeginDragDropSource()) {
    const UUID& uuid = gameObject.getUUID();
    ImGui::SetDragDropPayload(kGameObjectPayload, &uuid, sizeof(UUID));
    const SIZE_T selected = EditorSelection::getSelection().size();
    if (selected > 1 && EditorSelection::isSelected(gameObject)) {
      ImGui::Text("%u objects", static_cast<uint32>(selected));
    }
    else {
      ImGui::TextUnformatted(gameObject.getName().c_str());
    }
    ImGui::EndDragDropSource();
  }
}

/*
 */
SPtr<GameObject>
AssetDragDrop::getDraggedGameObject()
{
  const ImGuiPayload* payload = ImGui::GetDragDropPayload();
  return payload ? findPayloadGameObject(*payload) : nullptr;
}

/*
 */
SPtr<GameObject>
AssetDragDrop::acceptGameObject(int32 imguiFlags)
{
  const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kGameObjectPayload, imguiFlags);
  return payload ? findPayloadGameObject(*payload) : nullptr;
}

/*
 */
SPtr<GameObject>
AssetDragDrop::createPrefabObject(Scene& scene,
                                  const SPtr<PrefabAsset>& prefab,
                                  GameObject* parent)
{
  // In prefab mode the edited prefab would end up inside itself.
  const SPtr<PrefabAsset>& edited = EditorScene::getPrefab();
  if (!prefab || (edited && prefab->dependsOn(edited->getUUID()))) {
    CH_LOG_WARNING(AssetDragDropLog, "A prefab cannot be placed inside itself.");
    return nullptr;
  }

  const SPtr<GameObject> gameObject = GameObjectSerializer::instantiate(*prefab, scene, parent);
  EditorSelection::setSelectedGameObject(gameObject);
  EditorScene::recordCreated(*gameObject, "Place");
  return gameObject;
}

/*
 */
SPtr<GameObject>
AssetDragDrop::createModelObject(Scene& scene,
                                 const SPtr<ModelAsset>& modelAsset,
                                 GameObject* parent)
{
  if (!modelAsset || !modelAsset->getModel()) {
    CH_LOG_ERROR(AssetDragDropLog, "Model asset '{0}' has no model.",
                 modelAsset ? modelAsset->getName() : "null");
    return nullptr;
  }

  SPtr<GameObject> gameObject = scene.createGameObject(modelAsset->getName(), parent);
  gameObject->addComponent<ModelComponent>(modelAsset);

  // Alt places the whole model, whatever its import asked for; it can still be split later.
  const ModelExpandMode mode = ImGui::GetIO().KeyAlt
                                   ? ModelExpandMode::Single
                                   : modelAsset->getEditorData().importSettings.expandMode;
  if (mode != ModelExpandMode::Single) {
    ModelExpand::expand(scene, *gameObject, mode);
  }
  // A model with bones is there to be animated, so it comes ready for a clip.
  else if (modelAsset->getModel()->hasSkeleton()) {
    gameObject->addComponent<AnimatorComponent>();
  }
  EditorSelection::setSelectedGameObject(gameObject);
  EditorScene::recordCreated(*gameObject, "Place");
  return gameObject;
}

} // namespace chEngineSDK
