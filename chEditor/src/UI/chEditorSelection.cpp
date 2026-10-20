/************************************************************************/
/**
 * @file chEditorSelection.cpp
 * @author AccelMR
 * @date 2025/10/29
 * @details
 *  Selection management for the Chimera Editor.
 */
/************************************************************************/
#include "chEditorSelection.h"

#include "chAlgorithm.h"
#include "chScene.h"

namespace chEngineSDK {
namespace {
struct SelectionState
{
  Vector<SPtr<GameObject>> objects;
  // The Scene Graph asks for every row each frame, so a lookup must not walk the list.
  UnorderedSet<const GameObject*> lookup;
  // Picked in the Content Browser; selecting objects clears it, and the reverse.
  SPtr<IAsset> asset;
};

SelectionState&
getState()
{
  static SelectionState state;
  return state;
}

const SPtr<GameObject> kNoObject;

/*
 */
void
removeFromList(SelectionState& state, const GameObject& gameObject)
{
  if (state.lookup.erase(&gameObject) > 0) {
    Algorithm::removeFirstIf(state.objects, [&gameObject](const SPtr<GameObject>& object) {
      return object.get() == &gameObject;
    });
  }
}

/*
 */
void
addList(SelectionState& state, const Vector<SPtr<GameObject>>& objects)
{
  for (const SPtr<GameObject>& object : objects) {
    state.objects.push_back(object);
    state.lookup.insert(object.get());
    // The content of an instance comes from its prefab, so it is not selected with it.
    if (object->getPrefab().isNull()) {
      addList(state, object->getChildren());
    }
  }
}
} // namespace

/*
 */
const SPtr<GameObject>&
EditorSelection::getSelectedGameObject()
{
  const SelectionState& state = getState();
  return state.objects.empty() ? kNoObject : state.objects.back();
}

/*
 */
void
EditorSelection::setSelectedGameObject(const SPtr<GameObject>& gameObject)
{
  clear();
  add(gameObject);
}

/*
 */
const Vector<SPtr<GameObject>>&
EditorSelection::getSelection()
{
  return getState().objects;
}

/*
 */
bool
EditorSelection::isSelected(const GameObject& gameObject)
{
  return getState().lookup.contains(&gameObject);
}

/*
 */
void
EditorSelection::add(const SPtr<GameObject>& gameObject)
{
  if (!gameObject) {
    return;
  }
  SelectionState& state = getState();
  removeFromList(state, *gameObject);
  state.objects.push_back(gameObject);
  state.lookup.insert(gameObject.get());
  state.asset.reset();
}

/*
 */
void
EditorSelection::toggle(const SPtr<GameObject>& gameObject)
{
  if (!gameObject) {
    return;
  }
  if (isSelected(*gameObject)) {
    removeFromList(getState(), *gameObject);
  }
  else {
    add(gameObject);
  }
}

/*
 */
void
EditorSelection::clear()
{
  SelectionState& state = getState();
  state.objects.clear();
  state.lookup.clear();
}

/*
 */
void
EditorSelection::setSelectedAsset(const SPtr<IAsset>& asset)
{
  clear();
  getState().asset = asset;
}

/*
 */
const SPtr<IAsset>&
EditorSelection::getSelectedAsset()
{
  return getState().asset;
}

/*
 */
void
EditorSelection::getTopLevel(Vector<SPtr<GameObject>>& outObjects)
{
  outObjects.clear();
  const SelectionState& state = getState();
  for (const SPtr<GameObject>& object : state.objects) {
    bool bParentSelected = false;
    for (const GameObject* parent = object->getParent(); parent && !bParentSelected;
         parent = parent->getParent()) {
      bParentSelected = state.lookup.contains(parent);
    }
    if (!bParentSelected) {
      outObjects.push_back(object);
    }
  }
}

/*
 */
void
EditorSelection::selectAll(const Scene& scene, const GameObject* root)
{
  clear();
  getState().asset.reset();
  addList(getState(), root ? root->getChildren() : scene.getRootGameObjects());
}

/*
 */
void
EditorSelection::removeDetached()
{
  SelectionState& state = getState();
  for (SIZE_T i = state.objects.size(); i > 0; --i) {
    if (!state.objects[i - 1]->getScene()) {
      state.lookup.erase(state.objects[i - 1].get());
      state.objects.erase(state.objects.begin() + (i - 1));
    }
  }
}

} // namespace chEngineSDK
