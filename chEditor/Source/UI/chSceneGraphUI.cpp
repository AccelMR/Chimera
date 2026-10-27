/************************************************************************/
/**
 * @file chSceneGraphUI.cpp
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Scene graph UI class for the Chimera Editor.
 */
/************************************************************************/

#include "chSceneGraphUI.h"

#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chContentAssetUI.h"
#include "chEditorScene.h"
#include "chEditorCamera.h"
#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chLightComponent.h"
#include "chShapeComponent.h"
#include "chSkyLightComponent.h"
#include "chCameraComponent.h"
#include "chMath.h"
#include "chModelAsset.h"
#include "chModel.h"
#include "chModelComponent.h"
#include "chModelExpand.h"
#include "chPrefabAsset.h"
#include "chScene.h"
#include "chSceneManager.h"
#include "chStringUtils.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {

namespace {
const SPtr<GameObject> kNoParent;

/**
 * True when ancestor is gameObject or one of its parents, so deleting ancestor removes
 * gameObject too.
 */
bool
isInSubtree(const GameObject* gameObject, const GameObject& ancestor)
{
  for (; gameObject; gameObject = gameObject->getParent()) {
    if (gameObject == &ancestor) {
      return true;
    }
  }
  return false;
}

/*
 * The new object goes under parent (a root object when null) and is selected, so the
 * Inspector edits it at once.
 */
template <typename ComponentType>
void
createWithComponent(Scene& scene, const ANSICHAR* name, GameObject* parent)
{
  const SPtr<GameObject> gameObject = scene.createGameObject(name, parent);
  gameObject->addComponent<ComponentType>();
  EditorSelection::setSelectedGameObject(gameObject);
  EditorScene::recordCreated(*gameObject, "Create");
}

/*
 * A light, sky light or camera wins over a model on the same object, since that is what
 * such an object is for. Each component is compared once, so a big tree stays cheap to draw.
 */
const ANSICHAR*
getObjectIcon(const GameObject& gameObject)
{
  if (!gameObject.getPrefab().isNull()) {
    return ICON_FA_CUBES;
  }
  const ANSICHAR* icon = ICON_FA_DOT_CIRCLE;
  for (const UniquePtr<Component>& component : gameObject.getComponents()) {
    const UUID& typeId = component->getTypeId();
    if (typeId == TypeTraits<DirectionalLightComponent>::getTypeId()) {
      return ICON_FA_SUN;
    }
    if (typeId == TypeTraits<PointLightComponent>::getTypeId()) {
      return ICON_FA_LIGHTBULB;
    }
    if (typeId == TypeTraits<SpotLightComponent>::getTypeId()) {
      return ICON_FA_BULLSEYE;
    }
    if (typeId == TypeTraits<SkyLightComponent>::getTypeId()) {
      return ICON_FA_CLOUD_SUN;
    }
    if (typeId == TypeTraits<CameraComponent>::getTypeId()) {
      return ICON_FA_VIDEO;
    }
    if (typeId == TypeTraits<ModelComponent>::getTypeId()) {
      icon = ICON_FA_CUBE;
    }
    else if (typeId == TypeTraits<ShapeComponent>::getTypeId()) {
      icon = ICON_FA_SHAPES;
    }
  }
  return icon;
}

/*
 */
void
renderCreateLightItems(Scene& scene, GameObject* parent)
{
  if (!ImGui::BeginMenu(ICON_FA_LIGHTBULB "  Create Light")) {
    return;
  }
  if (ImGui::MenuItem(ICON_FA_SUN "  Directional Light")) {
    createWithComponent<DirectionalLightComponent>(scene, "Directional Light", parent);
  }
  if (ImGui::MenuItem(ICON_FA_LIGHTBULB "  Point Light")) {
    createWithComponent<PointLightComponent>(scene, "Point Light", parent);
  }
  if (ImGui::MenuItem(ICON_FA_BULLSEYE "  Spot Light")) {
    createWithComponent<SpotLightComponent>(scene, "Spot Light", parent);
  }
  if (ImGui::MenuItem(ICON_FA_CLOUD_SUN "  Sky Light")) {
    createWithComponent<SkyLightComponent>(scene, "Sky Light", parent);
  }
  ImGui::EndMenu();
}

/*
 */
void
renderCreateCameraItem(Scene& scene, GameObject* parent)
{
  if (ImGui::MenuItem(ICON_FA_VIDEO "  Create Camera")) {
    createWithComponent<CameraComponent>(scene, "Camera", parent);
  }
}

/*
 * The new shape goes under parent (a root object when null), named after its type, and is
 * selected, so the Inspector edits its sizes at once.
 */
void
renderCreateShapeItems(Scene& scene, GameObject* parent)
{
  if (!ImGui::BeginMenu(ICON_FA_SHAPES "  Create Shape")) {
    return;
  }
  for (uint32 i = 0; i < static_cast<uint32>(ShapeType::COUNT); ++i) {
    const ShapeType type = static_cast<ShapeType>(i);
    const ANSICHAR* name = MeshGenerator::getName(type);
    if (ImGui::MenuItem(name)) {
      const SPtr<GameObject> gameObject = scene.createGameObject(name, parent);
      gameObject->addComponent<ShapeComponent>(ShapeDesc{.type = type});
      EditorSelection::setSelectedGameObject(gameObject);
      EditorScene::recordCreated(*gameObject, "Create");
    }
  }
  ImGui::EndMenu();
}
} // namespace

/*
 */
void
SceneGraphUI::renderSceneGraphUI()
{
  const SPtr<Scene> scene = SceneManager::instance().getActiveScene().lock();
  const ANSICHAR* sceneName = scene ? scene->getName().c_str() : "No Active Scene";
  if (m_windowTitle.empty() || m_titleSceneName != sceneName) {
    m_titleSceneName = sceneName;
    // "###" keeps the window ID fixed, so ImGui keeps its position and docking when the
    // scene changes.
    m_windowTitle = StringUtils::format(ICON_FA_SITEMAP "  Scene Graph - {0}###SceneGraph",
                                        m_titleSceneName);
  }

  ImGui::Begin(m_windowTitle.c_str(), &m_isVisible);
  if (!scene) {
    ImGui::End();
    return;
  }

  handleKeys();
  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::InputTextWithHint("##search", ICON_FA_SEARCH "  Search", m_searchBuffer,
                           sizeof(m_searchBuffer));

  const GameObject* active = EditorSelection::getSelectedGameObject().get();
  m_revealTarget = active != m_lastSeenActive ? active : nullptr;
  m_visibleRows.clear();

  ImGui::BeginChild("##tree");
  // In prefab mode only the prefab is shown; the light that shows it is not part of it.
  const SPtr<GameObject>& prefabRoot = EditorScene::getPrefabRoot();
  if (m_searchBuffer[0] != '\0') {
    renderSearchResults(*scene,
                        prefabRoot ? prefabRoot->getChildren() : scene->getRootGameObjects());
  }
  else if (prefabRoot) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
    renderGameObject(*scene, prefabRoot, kNoParent, false);
  }
  else if (scene->getLevels().size() > 1) {
    // Only while sublevels are loaded (Play): a header per level, the map first.
    for (const SceneLevel& level : scene->getLevels()) {
      ImGui::PushID(static_cast<int32>(level.id));
      if (m_revealTarget && m_revealTarget->getLevel() == level.id) {
        ImGui::SetNextItemOpen(true);
      }
      const bool bOpen = ImGui::TreeNodeEx(
          "##level", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth,
          "%s  %s", level.id == Scene::MAP_LEVEL ? ICON_FA_MAP : ICON_FA_LAYER_GROUP,
          level.name.c_str());
      if (bOpen) {
        for (const SPtr<GameObject>& root : scene->getRootGameObjects()) {
          if (root->getLevel() == level.id) {
            renderGameObject(*scene, root, kNoParent, false);
          }
        }
        ImGui::TreePop();
      }
      ImGui::PopID();
    }
  }
  else {
    for (const SPtr<GameObject>& root : scene->getRootGameObjects()) {
      renderGameObject(*scene, root, kNoParent, false);
    }
  }
  if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
      !ImGui::IsAnyItemHovered()) {
    EditorSelection::clear();
  }
  renderEmptyAreaContextMenu(*scene);
  renderEmptyAreaDropTarget();
  ImGui::EndChild();

  applyPendingChanges(*scene);
  m_lastSeenActive = EditorSelection::getSelectedGameObject().get();
  ImGui::End();
}

/*
 */
void
SceneGraphUI::handleKeys()
{
  if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
      ImGui::GetIO().WantTextInput) {
    return;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
    beginRename(EditorSelection::getSelectedGameObject());
  }
  if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
    EditorScene::requestDelete();
  }
}

/*
 */
void
SceneGraphUI::applyPendingChanges(Scene& scene)
{
  if (m_pendingRangeEnd) {
    // Every row is drawn now, so both ends of the range have their place.
    constexpr SIZE_T kNotFound = static_cast<SIZE_T>(-1);
    SIZE_T anchor = kNotFound;
    SIZE_T end = kNotFound;
    for (SIZE_T i = 0; i < m_visibleRows.size(); ++i) {
      const GameObject* row = m_visibleRows[i]->get();
      anchor = row == m_rangeAnchor ? i : anchor;
      end = row == m_pendingRangeEnd ? i : end;
    }
    if (end != kNotFound && anchor != kNotFound) {
      EditorSelection::clear();
      for (SIZE_T i = Math::min(anchor, end); i <= Math::max(anchor, end); ++i) {
        EditorSelection::add(*m_visibleRows[i]);
      }
      EditorSelection::add(*m_visibleRows[end]);
    }
    else if (end != kNotFound) {
      EditorSelection::setSelectedGameObject(*m_visibleRows[end]);
    }
    m_pendingRangeEnd = nullptr;
  }

  if (m_pendingModel || m_pendingPrefab) {
    addPendingObject(scene);
  }
  if (m_pendingExpand) {
    EditorScene::recordChange(*m_pendingExpand, "Expand");
    if (ModelExpand::expand(scene, *m_pendingExpand, m_pendingExpandMode)) {
      m_openOnNextDraw = m_pendingExpand.get();
    }
    m_pendingExpand.reset();
  }
}

/*
 */
void
SceneGraphUI::renderSearchResults(Scene& scene, const Vector<SPtr<GameObject>>& gameObjects)
{
  for (const SPtr<GameObject>& gameObject : gameObjects) {
    if (StringUtils::containsIgnoreCase(gameObject->getName(), m_searchBuffer)) {
      renderGameObject(scene, gameObject, kNoParent, true);
    }
    renderSearchResults(scene, gameObject->getChildren());
  }
}

/*
 */
void
SceneGraphUI::renderGameObject(Scene& scene,
                               const SPtr<GameObject>& gameObject,
                               const SPtr<GameObject>& parent,
                               bool bFlat)
{
  m_visibleRows.push_back(&gameObject);
  const bool bSelected = EditorSelection::isSelected(*gameObject);

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                             ImGuiTreeNodeFlags_OpenOnDoubleClick |
                             ImGuiTreeNodeFlags_SpanAvailWidth;
  if (bFlat) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  }
  else if (gameObject->getChildren().empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }
  if (bSelected) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  // What is below an instance comes from its prefab, so it cannot be changed here: the
  // instance shows in blue and its content in gray, as in Unity.
  const GameObject* instance = gameObject->getOutermostPrefabInstance();
  const bool bInstanceRoot = instance == gameObject.get();
  const bool bInsideInstance = instance && !bInstanceRoot;
  const bool bLocked = instance != nullptr;
  const bool bPrefabRoot = gameObject == EditorScene::getPrefabRoot();

  // Objects can share a name, so the pointer keeps their tree nodes apart.
  ImGui::PushID(gameObject.get());
  if (gameObject.get() == m_openOnNextDraw) {
    // A model just split into parts, or a row that just got children, opens to show them.
    ImGui::SetNextItemOpen(true);
    m_openOnNextDraw = nullptr;
  }
  else if (!bFlat && m_revealTarget && m_revealTarget != gameObject.get() &&
           isInSubtree(m_revealTarget, *gameObject)) {
    ImGui::SetNextItemOpen(true);
  }
  if (bLocked) {
    ImGui::PushStyleColor(ImGuiCol_Text, bInstanceRoot ? ImVec4(0.45f, 0.7f, 1.0f, 1.0f)
                                                       : ImVec4(0.55f, 0.55f, 0.6f, 1.0f));
  }
  const bool bRenaming = m_renaming == gameObject;
  bool bOpen = false;
  if (bRenaming) {
    // Without the full width, so the name field after the icon gets the clicks.
    bOpen = ImGui::TreeNodeEx("##node", flags & ~ImGuiTreeNodeFlags_SpanAvailWidth, "%s",
                              getObjectIcon(*gameObject));
  }
  else {
    bOpen = ImGui::TreeNodeEx("##node", flags, "%s  %s", getObjectIcon(*gameObject),
                              gameObject->getName().c_str());
  }
  if (bLocked) {
    ImGui::PopStyleColor();
  }
  if (m_revealTarget == gameObject.get() && !ImGui::IsItemVisible()) {
    ImGui::SetScrollHereY(0.5f);
  }

  if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
    handleRowClick(gameObject, bSelected);
  }
  // The context menu acts on the selection, so it must hold the row it opens on.
  if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !bSelected) {
    EditorSelection::setSelectedGameObject(gameObject);
  }
  if (m_pendingSingleSelect == gameObject.get() &&
      ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] <
        io.MouseDragThreshold * io.MouseDragThreshold) {
      EditorSelection::setSelectedGameObject(gameObject);
      m_rangeAnchor = gameObject.get();
    }
    m_pendingSingleSelect = nullptr;
  }

  if (!bRenaming && !bInsideInstance && !bPrefabRoot) {
    // A drop acts on the selection, so a row dragged without being selected becomes it.
    if (!bSelected && ImGui::IsItemActive() &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      EditorSelection::setSelectedGameObject(gameObject);
    }
    AssetDragDrop::source(*gameObject);
  }

  if (ImGui::BeginDragDropTarget()) {
    if (!bLocked) {
      if (SPtr<ModelAsset> model = AssetDragDrop::acceptModel()) {
        m_pendingModel = std::move(model);
        m_pendingParent = gameObject;
      }
      if (SPtr<PrefabAsset> prefab = AssetDragDrop::acceptPrefab()) {
        m_pendingPrefab = std::move(prefab);
        m_pendingParent = gameObject;
      }
    }
    // A search result does not know its parent, the prefab's root has no siblings in prefab
    // mode, and nothing can be put next to the content of an instance.
    const bool bSiblingsAllowed =
        !bFlat && !bPrefabRoot && !(parent && parent->getOutermostPrefabInstance());
    acceptObjectDrop(gameObject, parent, bLocked, bSiblingsAllowed);
    ImGui::EndDragDropTarget();
  }

  // Inside the PushID of this object, so a fixed popup name is unique.
  if (ImGui::BeginPopupContextItem("GameObjectContext")) {
    renderObjectContextMenu(scene, gameObject, bLocked, bInsideInstance, bPrefabRoot);
    ImGui::EndPopup();
  }
  if (bRenaming) {
    renderRenameField(*gameObject);
  }

  if (bOpen && !bFlat) {
    for (const SPtr<GameObject>& child : gameObject->getChildren()) {
      renderGameObject(scene, child, gameObject, false);
    }
    ImGui::TreePop();
  }
  ImGui::PopID();
}

/*
 */
void
SceneGraphUI::handleRowClick(const SPtr<GameObject>& gameObject, bool bSelected)
{
  const ImGuiIO& io = ImGui::GetIO();
  if (io.KeyCtrl) {
    EditorSelection::toggle(gameObject);
    m_rangeAnchor = gameObject.get();
  }
  else if (io.KeyShift && m_rangeAnchor) {
    // Applied once every row is drawn, since the range may end below this row.
    m_pendingRangeEnd = gameObject.get();
  }
  else if (bSelected && EditorSelection::getSelection().size() > 1) {
    m_pendingSingleSelect = gameObject.get();
  }
  else {
    EditorSelection::setSelectedGameObject(gameObject);
    m_rangeAnchor = gameObject.get();
  }
}

/*
 */
void
SceneGraphUI::acceptObjectDrop(const SPtr<GameObject>& gameObject,
                               const SPtr<GameObject>& parent,
                               bool bLocked,
                               bool bSiblingsAllowed)
{
  const SPtr<GameObject> dragged = AssetDragDrop::getDraggedGameObject();
  if (!dragged) {
    return;
  }

  enum class DropPlace
  {
    Before,
    Inside,
    After
  };
  const ImVec2 min = ImGui::GetItemRectMin();
  const ImVec2 max = ImGui::GetItemRectMax();
  const float edge = (max.y - min.y) * 0.25f;
  const float mouseY = ImGui::GetMousePos().y;
  DropPlace place = DropPlace::Inside;
  if (bSiblingsAllowed && mouseY < min.y + edge) {
    place = DropPlace::Before;
  }
  else if (bSiblingsAllowed && mouseY > max.y - edge) {
    place = DropPlace::After;
  }

  // Refused before it highlights: the dragged object cannot go inside, before or after
  // itself or one of its children, nothing goes inside an instance, and a drop into its own
  // parent would change nothing. The other selected objects are checked when applied.
  if (isInSubtree(gameObject.get(), *dragged) ||
      (place == DropPlace::Inside && (bLocked || dragged->getParent() == gameObject.get()))) {
    return;
  }
  constexpr ImGuiDragDropFlags kFlags =
      ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
  if (!AssetDragDrop::acceptGameObject(kFlags)) {
    return;
  }

  ImDrawList& drawList = *ImGui::GetWindowDrawList();
  const ImU32 color = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
  if (place == DropPlace::Inside) {
    drawList.AddRect(min, max, color, 0.0f, 0, 2.0f);
  }
  else {
    const float lineY = place == DropPlace::Before ? min.y : max.y;
    drawList.AddLine(ImVec2(min.x, lineY), ImVec2(max.x, lineY), color, 2.0f);
  }

  if (!ImGui::GetDragDropPayload()->IsDelivery()) {
    return;
  }
  if (place == DropPlace::Inside) {
    EditorScene::requestReparent(gameObject, nullptr, false);
    m_openOnNextDraw = gameObject.get();
  }
  else {
    EditorScene::requestReparent(parent, gameObject, place == DropPlace::After);
  }
}

/*
 */
void
SceneGraphUI::renderObjectContextMenu(Scene& scene,
                                      const SPtr<GameObject>& gameObject,
                                      bool bLocked,
                                      bool bInsideInstance,
                                      bool bPrefabRoot)
{
  if (ImGui::MenuItem(ICON_FA_PLUS "  Create Child", nullptr, false, !bLocked)) {
    EditorScene::recordCreated(*scene.createGameObject("GameObject", gameObject.get()),
                               "Create");
  }
  if (!bLocked) {
    renderCreateShapeItems(scene, gameObject.get());
    renderCreateLightItems(scene, gameObject.get());
    renderCreateCameraItem(scene, gameObject.get());
  }
  const ModelComponent* model = gameObject->getComponent<ModelComponent>();
  if (!bLocked && model && model->isWholeModel() && model->getModel() &&
      !model->getModel()->hasSkeleton() &&
      ImGui::BeginMenu(ICON_FA_OBJECT_UNGROUP "  Expand Model")) {
    if (ImGui::MenuItem(ICON_FA_SITEMAP "  By Node")) {
      m_pendingExpand = gameObject;
      m_pendingExpandMode = ModelExpandMode::PerNode;
    }
    if (ImGui::MenuItem(ICON_FA_TH_LARGE "  By Mesh")) {
      m_pendingExpand = gameObject;
      m_pendingExpandMode = ModelExpandMode::PerMesh;
    }
    ImGui::EndMenu();
  }

  // From here on the items act on the whole selection, which holds this row.
  ImGui::Separator();
  if (ImGui::MenuItem(ICON_FA_PEN "  Rename", "F2", false, !bInsideInstance)) {
    beginRename(gameObject);
  }
  if (ImGui::MenuItem(ICON_FA_COPY "  Copy", "Ctrl+C")) {
    EditorScene::copySelection();
  }
  if (ImGui::MenuItem(ICON_FA_PASTE "  Paste", "Ctrl+V", false, EditorScene::canPaste())) {
    EditorScene::requestPaste();
  }
  if (ImGui::MenuItem(ICON_FA_CLONE "  Duplicate", "Ctrl+D", false,
                      EditorScene::canDuplicateOrDelete(*gameObject))) {
    EditorScene::requestDuplicate();
  }

  ImGui::Separator();
  if (ImGui::MenuItem(ICON_FA_CUBES "  Create Prefab", nullptr, false,
                      !bInsideInstance && m_contentBrowser)) {
    EditorScene::requestCreatePrefabs(m_contentBrowser->getCurrentFolderPath());
  }
  if (!gameObject->getPrefab().isNull()) {
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(gameObject->getPrefab());
    const SPtr<PrefabAsset> prefab = asset ? asset->as<PrefabAsset>() : nullptr;
    if (ImGui::MenuItem(ICON_FA_EDIT "  Edit Prefab", nullptr, false, prefab != nullptr)) {
      EditorScene::requestEditPrefab(prefab);
    }
    if (ImGui::MenuItem(ICON_FA_UNLINK "  Unpack Prefab", nullptr, false, !bInsideInstance)) {
      // The content stays; it is saved with the scene from now on.
      EditorScene::recordChange(*gameObject, "Unpack");
      gameObject->setPrefab(UUID::null());
    }
  }

  ImGui::Separator();
  if (ImGui::MenuItem(ICON_FA_TRASH_ALT "  Delete", "Del", false,
                      !bInsideInstance && !bPrefabRoot)) {
    EditorScene::requestDelete();
  }
}

/*
 */
void
SceneGraphUI::renderEmptyAreaContextMenu(Scene& scene)
{
  if (ImGui::BeginPopupContextWindow("EmptyAreaContextMenu_SceneGraph",
                                     ImGuiPopupFlags_MouseButtonRight |
                                         ImGuiPopupFlags_NoOpenOverItems)) {
    GameObject* parent = EditorScene::getDefaultParent();
    if (ImGui::MenuItem(ICON_FA_PLUS "  Create Empty")) {
      EditorScene::recordCreated(*scene.createGameObject("GameObject", parent), "Create");
    }
    renderCreateShapeItems(scene, parent);
    renderCreateLightItems(scene, parent);
    renderCreateCameraItem(scene, parent);
    if (ImGui::MenuItem(ICON_FA_PASTE "  Paste", "Ctrl+V", false, EditorScene::canPaste())) {
      // Pasted where objects made without a parent go.
      EditorSelection::clear();
      EditorScene::requestPaste();
    }
    ImGui::EndPopup();
  }
}

/*
 */
void
SceneGraphUI::renderEmptyAreaDropTarget()
{
  // Only while dragging: at other times the button would cover the empty area and keep
  // its context menu from opening.
  if (!ImGui::GetDragDropPayload()) {
    return;
  }

  const ImVec2 available = ImGui::GetContentRegionAvail();
  ImGui::InvisibleButton("##emptyAreaDrop", ImVec2(Math::max(available.x, 1.0f),
                                                   Math::max(available.y,
                                                             ImGui::GetFrameHeight())));
  if (!ImGui::BeginDragDropTarget()) {
    return;
  }
  if (SPtr<ModelAsset> model = AssetDragDrop::acceptModel()) {
    m_pendingModel = std::move(model);
    m_pendingParent.reset();
  }
  if (SPtr<PrefabAsset> prefab = AssetDragDrop::acceptPrefab()) {
    m_pendingPrefab = std::move(prefab);
    m_pendingParent.reset();
  }
  // In prefab mode the empty area stands for the prefab's root.
  const SPtr<GameObject>& defaultParent = EditorScene::getPrefabRoot();
  const SPtr<GameObject> dragged = AssetDragDrop::getDraggedGameObject();
  if (dragged && dragged->getParent() != defaultParent.get() &&
      AssetDragDrop::acceptGameObject()) {
    EditorScene::requestReparent(defaultParent, nullptr, false);
  }
  ImGui::EndDragDropTarget();
}

/*
 */
void
SceneGraphUI::addPendingObject(Scene& scene)
{
  GameObject* parent =
      m_pendingParent ? m_pendingParent.get() : EditorScene::getDefaultParent();
  const SPtr<GameObject> gameObject =
      m_pendingModel ? AssetDragDrop::createModelObject(scene, m_pendingModel, parent)
                     : AssetDragDrop::createPrefabObject(scene, m_pendingPrefab, parent);
  m_pendingModel.reset();
  m_pendingPrefab.reset();
  m_pendingParent.reset();
  if (!gameObject || !m_editorCamera) {
    return;
  }

  // Under a moved parent the render items are only in place after the transform update.
  scene.updateTransforms();
  AABox bounds(Vector3::ZERO, Vector3::ZERO);
  if (gameObject->getHierarchyWorldBounds(bounds)) {
    m_editorCamera->focus(bounds);
  }
}

/*
 */
void
SceneGraphUI::beginRename(const SPtr<GameObject>& gameObject)
{
  if (!gameObject) {
    return;
  }
  // What is inside an instance takes its names from the prefab.
  const GameObject* instance = gameObject->getOutermostPrefabInstance();
  if (instance && instance != gameObject.get()) {
    return;
  }
  m_renaming = gameObject;
  StringUtils::copyToBuffer(m_renameBuffer, gameObject->getName());
  m_bRenameFocusRequested = true;
}

/*
 * The name changes once, when Enter is pressed or the field is left, so a rename is one undo
 * step. Escape keeps the old name.
 */
void
SceneGraphUI::renderRenameField(GameObject& gameObject)
{
  ImGui::SameLine();
  ImGui::SetNextItemWidth(-FLT_MIN);
  if (m_bRenameFocusRequested) {
    ImGui::SetKeyboardFocusHere();
    m_bRenameFocusRequested = false;
  }
  const bool bEnter = ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer),
                                       ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_AutoSelectAll);
  if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    m_renaming.reset();
    return;
  }
  if (!bEnter && !ImGui::IsItemDeactivated()) {
    return;
  }

  const StringView name = StringUtils::trimView(StringView(m_renameBuffer));
  if (!name.empty() && name != gameObject.getName()) {
    EditorScene::recordChange(gameObject, "Rename");
    gameObject.setName(String(name));
  }
  m_renaming.reset();
}

} // namespace chEngineSDK
