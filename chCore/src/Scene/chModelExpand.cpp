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
      gameObject.addComponent<ModelComponent>(source.model, nodeIndex, meshIndex, pivot);
  for (const uint32 slot : part.getUsedMaterialSlots()) {
    if (source.overrides[slot]) {
      part.setMaterial(slot, source.overrides[slot]);
    }
  }
  part.setEnabled(source.bEnabled);
}

/*
 * Nodes without meshes become empty objects, so the hierarchy below them stays.
 */
void
addNodeObject(Scene& scene,
              const ExpandSource& source,
              const UnorderedMap<const ModelNode*, uint32>& nodeIndices,
              const ModelNode& node,
              GameObject& parent)
{
  const SPtr<GameObject> gameObject = scene.createGameObject(node.getName(), &parent);
  // A root node's global transform also holds the model's own transform.
  setLocalTransform(*gameObject, node.getParent() ? node.getLocalTransform()
                                                  : node.getGlobalTransform());
  if (!node.getMeshes().empty()) {
    addPart(source, *gameObject, nodeIndices.at(&node), ModelComponent::ALL, Vector3::ZERO);
  }
  for (const ModelNode* child : node.getChildren()) {
    addNodeObject(scene, source, nodeIndices, *child, *gameObject);
  }
}

/*
 */
void
expandPerNode(Scene& scene, const ExpandSource& source, GameObject& root)
{
  const Vector<ModelNode*>& nodes = source.model->getAllNodes();
  UnorderedMap<const ModelNode*, uint32> nodeIndices;
  nodeIndices.reserve(nodes.size());
  for (SIZE_T index = 0; index < nodes.size(); ++index) {
    nodeIndices.emplace(nodes[index], static_cast<uint32>(index));
  }

  for (const ModelNode* node : source.model->getRootNodes()) {
    addNodeObject(scene, source, nodeIndices, *node, root);
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
  const Vector<ModelNode*>& nodes = source.model->getAllNodes();
  const Vector<ModelMaterialSlot>& slots = source.model->getMaterialSlots();
  for (SIZE_T nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
    const ModelNode& node = *nodes[nodeIndex];
    const Vector<SPtr<Mesh>>& meshes = node.getMeshes();
    for (SIZE_T meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
      const Mesh& mesh = *meshes[meshIndex];
      // A node with several meshes repeats its name, so the material tells them apart.
      String name(node.getName());
      const uint32 slot = mesh.getMaterialSlot();
      if (meshes.size() > 1) {
        name = slot < slots.size() && !slots[slot].name.empty()
                   ? StringUtils::format("{0} - {1}", name, slots[slot].name)
                   : StringUtils::format("{0} - {1}", name, meshIndex);
      }

      const Vector3 pivot = mesh.getBounds().center;
      const SPtr<GameObject> gameObject = scene.createGameObject(name, &root);
      setLocalTransform(*gameObject, TranslationMatrix(pivot) * node.getGlobalTransform());
      addPart(source, *gameObject, static_cast<uint32>(nodeIndex),
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
  if (mode == ModelExpandMode::Single || !whole || !whole->isWholeModel() ||
      !whole->getModel()) {
    return false;
  }

  ExpandSource source{.model = whole->getModel(), .overrides = {}, .bEnabled = true};
  source.model->updateTransforms();
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
