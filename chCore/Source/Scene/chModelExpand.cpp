/************************************************************************/
/**
 * @file chModelExpand.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Splits an object that draws a whole model into one object per node or per mesh.
 */
/************************************************************************/
#include "chModelExpand.h"

#include "chGameObject.h"
#include "chMaterialAsset.h"
#include "chMatrixHelpers.h"
#include "chMesh.h"
#include "chModel.h"
#include "chModelComponent.h"
#include "chQuaternion.h"
#include "chScene.h"
#include "chStringUtils.h"

namespace chEngineSDK {
namespace {
/*
 * What the parts take over from the whole-model component it replaces.
 */
struct ExpandSource
{
  SPtr<ModelAsset> modelAsset;
  SPtr<Model> model;
  // One per material slot of the model; null uses the model's material.
  Vector<SPtr<MaterialAsset>> overrides;
  bool bEnabled = true;
};

/*
 */
void
setLocalTransform(GameObject& gameObject, const Matrix4& localMatrix)
{
  // decompose leaves these as they are for a zero scale, so such a node gets no transform.
  Vector3 scale = Vector3::UNIT;
  Quaternion rotation = Quaternion::IDENTITY;
  Vector3 translation = Vector3::ZERO;
  if (!localMatrix.decompose(scale, rotation, translation)) {
    scale = Vector3::UNIT;
  }

  Transform& transform = gameObject.getTransform();
  transform.setLocalPosition(translation);
  transform.setLocalRotation(rotation);
  transform.setLocalScale(scale);
}

/*
 */
void
addPart(const ExpandSource& source,
        GameObject& gameObject,
        uint32 nodeIndex,
        uint32 meshIndex,
        const Vector3& pivot)
{
  ModelComponent& part =
      gameObject.addComponent<ModelComponent>(source.modelAsset, nodeIndex, meshIndex, pivot);
  for (const uint32 slot : part.getUsedMaterialSlots()) {
    if (source.overrides[slot]) {
      part.setMaterial(slot, source.overrides[slot]);
    }
  }
  part.setEnabled(source.bEnabled);
}

/*
 * Parents come before their children in the model, so each node's object can be made
 * under its parent's in one pass. Nodes without meshes become empty objects, so the
 * hierarchy below them stays.
 */
void
expandPerNode(Scene& scene, const ExpandSource& source, GameObject& root)
{
  const Vector<ModelNode>& nodes = source.model->getNodes();
  Vector<GameObject*> objects(nodes.size(), nullptr);
  for (uint32 nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
    const ModelNode& node = nodes[nodeIndex];
    GameObject* parent = node.parent == ModelNode::NO_PARENT ? &root : objects[node.parent];
    const SPtr<GameObject> gameObject = scene.createGameObject(node.name, parent);
    objects[nodeIndex] = gameObject.get();

    Transform& transform = gameObject->getTransform();
    transform.setLocalPosition(node.position);
    transform.setLocalRotation(node.rotation);
    transform.setLocalScale(node.scale);
    if (node.meshCount > 0) {
      addPart(source, *gameObject, nodeIndex, ModelComponent::ALL, Vector3::ZERO);
    }
  }
}

/*
 * Every mesh becomes a direct child of root, placed where the whole model drew it. The
 * object stands at the center of the mesh so the gizmo shows on the part, not at the
 * model's origin.
 */
void
expandPerMesh(Scene& scene, const ExpandSource& source, GameObject& root)
{
  const Vector<ModelNode>& nodes = source.model->getNodes();
  const Vector<SPtr<Mesh>>& modelMeshes = source.model->getMeshes();
  const Vector<ModelMaterialSlot>& slots = source.model->getMaterialSlots();
  for (uint32 nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
    const ModelNode& node = nodes[nodeIndex];
    const Span<const uint32> meshes = source.model->getNodeMeshes(nodeIndex);
    for (SIZE_T meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
      const Mesh& mesh = *modelMeshes[meshes[meshIndex]];
      // A node with several meshes repeats its name, so the material tells them apart.
      String name(node.name);
      const uint32 slot = mesh.getMaterialSlot();
      if (meshes.size() > 1) {
        name = slot < slots.size() && !slots[slot].name.empty()
                   ? StringUtils::format("{0} - {1}", name, slots[slot].name)
                   : StringUtils::format("{0} - {1}", name, meshIndex);
      }

      const Vector3 pivot = mesh.getBounds().center;
      const SPtr<GameObject> gameObject = scene.createGameObject(name, &root);
      setLocalTransform(*gameObject,
                        TranslationMatrix(pivot) * source.model->getNodeRestMatrix(nodeIndex));
      addPart(source, *gameObject, nodeIndex,
              static_cast<uint32>(meshIndex), pivot);
    }
  }
}
} // namespace

/*
 */
bool
ModelExpand::expand(Scene& scene, GameObject& gameObject, ModelExpandMode mode)
{
  ModelComponent* whole = gameObject.getComponent<ModelComponent>();
  // Bones belong to the model as a whole, so a model with a skeleton is never split.
  if (mode == ModelExpandMode::Single || !whole || !whole->isWholeModel() ||
      !whole->getModel() || whole->getModel()->hasSkeleton()) {
    return false;
  }

  ExpandSource source{.modelAsset = whole->getModelAsset(),
                      .model = whole->getModel(),
                      .overrides = {},
                      .bEnabled = true};
  source.overrides.resize(source.model->getMaterialSlots().size());
  for (const uint32 slot : whole->getUsedMaterialSlots()) {
    source.overrides[slot] = whole->getMaterialOverride(slot);
  }
  source.bEnabled = whole->isEnabled();
  gameObject.removeComponent(*whole);

  if (mode == ModelExpandMode::PerNode) {
    expandPerNode(scene, source, gameObject);
  }
  else {
    expandPerMesh(scene, source, gameObject);
  }
  return true;
}

} // namespace chEngineSDK
