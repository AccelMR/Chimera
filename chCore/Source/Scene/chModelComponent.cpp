/************************************************************************/
/**
 * @file chModelComponent.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  Component that draws a Model, or one part of it, at its GameObject.
 */
/************************************************************************/
#include "chModelComponent.h"

#include "chAlgorithm.h"
#include "chAnimationClipAsset.h"
#include "chAssetManager.h"
#include "chFileStream.h"
#include "chGameObject.h"
#include "chMaterialAsset.h"
#include "chMath.h"
#include "chMatrixHelpers.h"
#include "chModel.h"
#include "chModelAsset.h"
#include "chScene.h"
#include "chSceneManager.h"

namespace chEngineSDK {
namespace {
// Returned by reference for slots that do not exist.
const SPtr<MaterialAsset> kNoMaterial;

// Above the slots of any real model; only a broken file asks for more.
constexpr uint32 kMaxOverrides = 65536;

struct ModelComponentFileData
{
  UUID modelAsset;
  uint32 nodeIndex;
  uint32 meshIndex;
  float pivot[3];
  uint32 overrideCount;
};
static_assert(sizeof(ModelComponentFileData) == 40,
              "ModelComponentFileData must have no padding");

struct MaterialOverrideFileData
{
  UUID material;
  uint32 slot;
};
static_assert(sizeof(MaterialOverrideFileData) == 20,
              "MaterialOverrideFileData must have no padding");

// Version 2: after the overrides.
struct AnimationFileData
{
  UUID clip;
  float playbackSpeed;
  uint32 flags;
};
static_assert(sizeof(AnimationFileData) == 24, "AnimationFileData must have no padding");

constexpr uint32 kAnimationPlaying = 1u << 0;
constexpr uint32 kAnimationLooping = 1u << 1;

// The last revision given to a skinned render item. Main thread only, like the scene.
uint64 g_skinRevision = 0;

/*
 * Null when there is no such asset or it is of another type. Without an AssetManager
 * (tests) nothing is found.
 */
template<typename T>
NODISCARD SPtr<T>
findAsset(const UUID& id)
{
  if (id.isNull() || !AssetManager::isStarted()) {
    return nullptr;
  }
  const SPtr<IAsset> asset = AssetManager::instance().getAsset(id);
  return asset ? asset->as<T>() : nullptr;
}

/*
 * Calls visit(mesh, localMatrix, node) for every mesh the part draws, localMatrix being where
 * the mesh sits relative to the GameObject in the rest pose and node the model node that
 * draws it (ALL for a part). An index past the end draws nothing.
 */
template<typename Visitor>
void
forEachDrawnMesh(const Model& model,
                 uint32 nodeIndex,
                 uint32 meshIndex,
                 const Vector3& pivot,
                 Visitor&& visit)
{
  const Vector<SPtr<Mesh>>& meshes = model.getMeshes();
  if (nodeIndex == ModelComponent::ALL) {
    for (uint32 node = 0; node < model.getNodeCount(); ++node) {
      for (const uint32 index : model.getNodeMeshes(node)) {
        visit(*meshes[index], model.getNodeRestMatrix(node), node);
      }
    }
    return;
  }
  if (nodeIndex >= model.getNodeCount()) {
    return;
  }

  // The node's own transform went into the GameObject; only the pivot is left.
  const Matrix4 localMatrix = TranslationMatrix(-pivot);
  const Span<const uint32> nodeMeshes = model.getNodeMeshes(nodeIndex);
  if (meshIndex == ModelComponent::ALL) {
    for (const uint32 index : nodeMeshes) {
      visit(*meshes[index], localMatrix, ModelComponent::ALL);
    }
  }
  else if (meshIndex < nodeMeshes.size()) {
    visit(*meshes[nodeMeshes[meshIndex]], localMatrix, ModelComponent::ALL);
  }
}
} // namespace

/*
 */
ModelComponent::ModelComponent(const SPtr<ModelAsset>& modelAsset)
{
  setModel(modelAsset);
}

/*
 */
ModelComponent::ModelComponent(const SPtr<ModelAsset>& modelAsset,
                               uint32 nodeIndex,
                               uint32 meshIndex,
                               const Vector3& pivot)
{
  setModelPart(modelAsset, nodeIndex, meshIndex, pivot);
}

/*
 */
void
ModelComponent::setModel(const SPtr<ModelAsset>& modelAsset)
{
  setModelPart(modelAsset, ALL, ALL, Vector3::ZERO);
}

/*
 */
void
ModelComponent::setModelPart(const SPtr<ModelAsset>& modelAsset,
                             uint32 nodeIndex,
                             uint32 meshIndex,
                             const Vector3& pivot)
{
  if (isRegistered()) {
    removeRenderItems();
  }

  m_modelAsset = modelAsset;
  m_model = modelAsset ? modelAsset->getModel() : nullptr;
  m_modelAssetId = modelAsset ? modelAsset->getUUID() : UUID::null();
  m_nodeIndex = nodeIndex;
  m_meshIndex = meshIndex;
  m_pivot = pivot;
  rebuildUsedSlots();
  bindAnimation();

  if (isRegistered()) {
    addRenderItems();
  }
}

/*
 */
void
ModelComponent::rebuildUsedSlots()
{
  m_usedSlots.clear();
  m_materialOverrides.clear();
  if (!m_model) {
    return;
  }

  const SIZE_T slotCount = m_model->getMaterialSlots().size();
  forEachDrawnMesh(*m_model, m_nodeIndex, m_meshIndex, m_pivot,
                   [this, slotCount](const Mesh& mesh, const Matrix4&, uint32) {
                     const uint32 slot = mesh.getMaterialSlot();
                     if (slot < slotCount && !Algorithm::contains(m_usedSlots, slot)) {
                       m_usedSlots.push_back(slot);
                     }
                   });
  Algorithm::sort(m_usedSlots);
  m_materialOverrides.resize(m_usedSlots.size());
}

/*
 */
uint32
ModelComponent::findUsedSlot(uint32 slot) const
{
  const SIZE_T index = Algorithm::lowerBound(m_usedSlots, slot);
  if (index < m_usedSlots.size() && m_usedSlots[index] == slot) {
    return static_cast<uint32>(index);
  }
  return ALL;
}

/*
 */
void
ModelComponent::setMaterial(uint32 slot, const SPtr<MaterialAsset>& material)
{
  const uint32 index = findUsedSlot(slot);
  if (index == ALL) {
    return;
  }

  m_materialOverrides[index] = material;
  if (!isRegistered()) {
    return;
  }

  Scene& scene = *getOwner()->getScene();
  const Material* renderMaterial = getRenderMaterial(slot);
  for (const MeshPart& part : m_parts) {
    if (part.mesh->getMaterialSlot() == slot) {
      scene.getRenderItem(part.renderItemId).material = renderMaterial;
    }
  }
}

/*
 */
const SPtr<MaterialAsset>&
ModelComponent::getMaterialOverride(uint32 slot) const
{
  const uint32 index = findUsedSlot(slot);
  return index != ALL ? m_materialOverrides[index] : kNoMaterial;
}

/*
 */
const SPtr<MaterialAsset>&
ModelComponent::getMaterial(uint32 slot) const
{
  if (!m_model || slot >= m_model->getMaterialSlots().size()) {
    return kNoMaterial;
  }
  const SPtr<MaterialAsset>& materialOverride = getMaterialOverride(slot);
  if (materialOverride) {
    return materialOverride;
  }
  return m_model->getMaterialSlots()[slot].material;
}

/*
 */
const Material*
ModelComponent::getRenderMaterial(uint32 slot) const
{
  const SPtr<MaterialAsset>& material = getMaterial(slot);
  return material ? &material->getMaterial() : nullptr;
}

/*
 */
bool
ModelComponent::getWorldBounds(AABox& outBounds) const
{
  if (!isRegistered() || m_parts.empty()) {
    return false;
  }

  Scene& scene = *getOwner()->getScene();
  outBounds = scene.getRenderItem(m_parts[0].renderItemId).worldBounds.getBox();
  for (const MeshPart& part : Span<const MeshPart>(m_parts).subspan(1)) {
    outBounds += scene.getRenderItem(part.renderItemId).worldBounds.getBox();
  }
  return true;
}

/*
 */
void
ModelComponent::setAnimation(const SPtr<AnimationClipAsset>& clip)
{
  m_animationAsset = clip;
  m_animation = clip ? clip->getClip() : nullptr;
  m_animationAssetId = clip ? clip->getUUID() : UUID::null();
  m_animationTime = 0.0f;
  bindAnimation();
  // Also when the new clip moves nothing: the parts go back to the rest pose.
  refreshPartMatrices();
}

/*
 */
void
ModelComponent::setPlaying(bool bPlaying)
{
  if (bPlaying && !m_bPlaying && !m_bLooping && m_animation) {
    const float duration = m_animation->getDuration();
    if (m_playbackSpeed >= 0.0f && m_animationTime >= duration) {
      setAnimationTime(0.0f);
    }
    else if (m_playbackSpeed < 0.0f && m_animationTime <= 0.0f) {
      setAnimationTime(duration);
    }
  }
  m_bPlaying = bPlaying;
}

/*
 */
void
ModelComponent::setAnimationTime(float time)
{
  m_animationTime = m_animation ? Math::clamp(time, 0.0f, m_animation->getDuration()) : 0.0f;
  m_bPoseDirty = true;
}

/*
 */
const Matrix4&
ModelComponent::getNodeMatrix(uint32 nodeIndex) const
{
  return isAnimating() ? m_poseMatrices[nodeIndex] : m_model->getNodeRestMatrix(nodeIndex);
}

/*
 */
void
ModelComponent::update(float deltaTime)
{
  if (!isAnimating()) {
    return;
  }

  if (m_bPlaying && m_playbackSpeed != 0.0f) {
    const float duration = m_animation->getDuration();
    float time = m_animationTime + deltaTime * m_playbackSpeed;
    if (m_bLooping && duration > 0.0f) {
      time -= Math::floor(time / duration) * duration;
    }
    else if (time >= duration || time <= 0.0f) {
      time = Math::clamp(time, 0.0f, duration);
      m_bPlaying = false;
    }
    m_animationTime = time;
    m_bPoseDirty = true;
  }

  if (m_bPoseDirty) {
    m_bPoseDirty = false;
    computePose();
    refreshPartMatrices();
  }
}

/*
 */
void
ModelComponent::bindAnimation()
{
  m_trackNodes.clear();
  m_localPose.clear();
  m_poseMatrices.clear();
  m_bPoseDirty = false;
  if (!m_model || !m_animation || !isWholeModel()) {
    return;
  }

  // Once per clip change, so a walk over the nodes per track is fine.
  const Vector<ModelNode>& nodes = m_model->getNodes();
  bool bAnyNode = false;
  m_trackNodes.reserve(m_animation->getTracks().size());
  for (const AnimationTrack& track : m_animation->getTracks()) {
    uint32 target = AnimationClip::NO_TARGET;
    for (uint32 i = 0; i < nodes.size(); ++i) {
      if (nodes[i].name == track.nodeName) {
        target = i;
        break;
      }
    }
    m_trackNodes.push_back(target);
    bAnyNode = bAnyNode || target != AnimationClip::NO_TARGET;
  }
  if (!bAnyNode) {
    m_trackNodes.clear();
    return;
  }

  // Nodes no track moves keep their rest transform for good.
  m_localPose.resize(nodes.size());
  for (SIZE_T i = 0; i < nodes.size(); ++i) {
    m_localPose[i] = {
        .position = nodes[i].position, .rotation = nodes[i].rotation, .scale = nodes[i].scale};
  }
  m_poseMatrices.resize(nodes.size());
  computePose();
}

/*
 */
void
ModelComponent::computePose()
{
  m_animation->sample(m_animationTime, m_trackNodes, m_localPose);
  const Vector<ModelNode>& nodes = m_model->getNodes();
  // Parents come first, so each parent's matrix is ready when its children need it.
  for (SIZE_T i = 0; i < nodes.size(); ++i) {
    const NodeTransform& local = m_localPose[i];
    const Matrix4 localMatrix =
        ScaleRotationTranslationMatrix(local.scale, local.rotation, local.position);
    m_poseMatrices[i] = nodes[i].parent == ModelNode::NO_PARENT
                            ? localMatrix
                            : localMatrix * m_poseMatrices[nodes[i].parent];
  }
}

/*
 */
void
ModelComponent::refreshPartMatrices()
{
  // Parts only exist while registered.
  if (m_parts.empty()) {
    return;
  }
  for (MeshPart& part : m_parts) {
    if (part.node != ALL) {
      part.localMatrix = getNodeMatrix(part.node);
    }
  }
  updateSkinMatrices();
  updateRenderItems();
}

/*
 */
void
ModelComponent::serialize(DataStream& stream) const
{
  uint32 overrideCount = 0;
  for (const SPtr<MaterialAsset>& material : m_materialOverrides) {
    overrideCount += material ? 1 : 0;
  }

  const uint32 version = SERIALIZATION_VERSION;
  const ModelComponentFileData data{.modelAsset = m_modelAssetId,
                                    .nodeIndex = m_nodeIndex,
                                    .meshIndex = m_meshIndex,
                                    .pivot = {m_pivot.x, m_pivot.y, m_pivot.z},
                                    .overrideCount = overrideCount};
  stream.write(&version, sizeof(version));
  stream.write(&data, sizeof(data));
  for (SIZE_T i = 0; i < m_usedSlots.size(); ++i) {
    if (m_materialOverrides[i]) {
      const MaterialOverrideFileData materialOverride{
          .material = m_materialOverrides[i]->getUUID(), .slot = m_usedSlots[i]};
      stream.write(&materialOverride, sizeof(materialOverride));
    }
  }

  const AnimationFileData animation{
      .clip = m_animationAssetId,
      .playbackSpeed = m_playbackSpeed,
      .flags = (m_bPlaying ? kAnimationPlaying : 0u) | (m_bLooping ? kAnimationLooping : 0u)};
  stream.write(&animation, sizeof(animation));
}

/*
 */
bool
ModelComponent::deserialize(DataStream& stream)
{
  uint32 version = 0;
  ModelComponentFileData data;
  if (stream.read(&version, sizeof(version)) != sizeof(version) || version == 0 ||
      version > SERIALIZATION_VERSION || stream.read(&data, sizeof(data)) != sizeof(data) ||
      data.overrideCount > kMaxOverrides) {
    return false;
  }

  const SPtr<ModelAsset> modelAsset = findAsset<ModelAsset>(data.modelAsset);
  setModelPart(modelAsset, data.nodeIndex, data.meshIndex,
               Vector3(data.pivot[0], data.pivot[1], data.pivot[2]));
  m_modelAssetId = data.modelAsset;
  if (!modelAsset && !data.modelAsset.isNull()) {
    CH_LOG_WARNING(SceneManagerLog, "A model component lost its model {0}.", data.modelAsset);
  }

  for (uint32 i = 0; i < data.overrideCount; ++i) {
    MaterialOverrideFileData materialOverride;
    if (stream.read(&materialOverride, sizeof(materialOverride)) != sizeof(materialOverride)) {
      return false;
    }
    const SPtr<MaterialAsset> material = findAsset<MaterialAsset>(materialOverride.material);
    if (material) {
      setMaterial(materialOverride.slot, material);
    }
    else if (modelAsset) {
      CH_LOG_WARNING(SceneManagerLog, "A model component lost its material {0}.",
                     materialOverride.material);
    }
  }

  if (version < 2) {
    return true;
  }
  AnimationFileData animation;
  if (stream.read(&animation, sizeof(animation)) != sizeof(animation)) {
    return false;
  }
  const SPtr<AnimationClipAsset> clip = findAsset<AnimationClipAsset>(animation.clip);
  setAnimation(clip);
  m_animationAssetId = animation.clip;
  m_playbackSpeed = animation.playbackSpeed;
  m_bPlaying = (animation.flags & kAnimationPlaying) != 0;
  m_bLooping = (animation.flags & kAnimationLooping) != 0;
  if (!clip && !animation.clip.isNull()) {
    CH_LOG_WARNING(SceneManagerLog, "A model component lost its animation {0}.",
                   animation.clip);
  }
  return true;
}

/*
 */
void
ModelComponent::collectReferences(Vector<UUID>& outReferences) const
{
  if (!m_modelAssetId.isNull()) {
    outReferences.push_back(m_modelAssetId);
  }
  for (const SPtr<MaterialAsset>& material : m_materialOverrides) {
    if (material) {
      outReferences.push_back(material->getUUID());
    }
  }
  if (!m_animationAssetId.isNull()) {
    outReferences.push_back(m_animationAssetId);
  }
}

/*
 */
void
ModelComponent::onRegister()
{
  addRenderItems();
}

/*
 */
void
ModelComponent::onUnregister()
{
  removeRenderItems();
}

/*
 */
void
ModelComponent::onTransformChanged()
{
  updateRenderItems();
}

/*
 */
void
ModelComponent::updateSkinMatrices()
{
  Scene& scene = *getOwner()->getScene();
  for (const MeshPart& part : m_parts) {
    if (part.firstSkinMatrix == ALL) {
      continue;
    }
    // Row vectors: the bind pose takes the vertex into the bone's space first, then the
    // bone's pose takes it into the model's.
    const MeshSkin& skin = part.mesh->getSkin();
    for (SIZE_T bone = 0; bone < skin.boneNodes.size(); ++bone) {
      m_skinMatrices[part.firstSkinMatrix + bone] =
          skin.inverseBindMatrices[bone] * getNodeMatrix(skin.boneNodes[bone]);
    }
    scene.getRenderItem(part.renderItemId).skinRevision = ++g_skinRevision;
  }
}

/*
 */
void
ModelComponent::updateRenderItems()
{
  Scene& scene = *getOwner()->getScene();
  const Matrix4& ownerWorld = getOwner()->getTransform().getWorldMatrix();
  for (const MeshPart& part : m_parts) {
    RenderItem& item = scene.getRenderItem(part.renderItemId);
    if (part.firstSkinMatrix == ALL) {
      item.worldMatrix = part.localMatrix * ownerWorld;
      item.worldBounds = part.mesh->getBounds().getTransformed(item.worldMatrix);
      continue;
    }

    // The skinned vertices are already in the model's space.
    item.worldMatrix = ownerWorld;
    const MeshSkin& skin = part.mesh->getSkin();
    bool bFirst = true;
    AABox bounds(Vector3::ZERO, Vector3::ZERO);
    for (const SkinBoneBounds& boneBounds : part.mesh->getSkinBoneBounds()) {
      const AABox box = boneBounds.box.getTransformed(
          getNodeMatrix(skin.boneNodes[boneBounds.bone]) * ownerWorld);
      if (bFirst) {
        bounds = box;
        bFirst = false;
      }
      else {
        bounds += box;
      }
    }
    item.worldBounds = bFirst ? part.mesh->getBounds().getTransformed(part.localMatrix *
                                                                      ownerWorld)
                              : SphereBoxBounds(bounds);
  }
}

/*
 */
void
ModelComponent::addRenderItems()
{
  if (!m_model) {
    return;
  }

  // The parts first, so the skin matrices are sized before any render item points at them.
  uint32 skinMatrixCount = 0;
  forEachDrawnMesh(
      *m_model, m_nodeIndex, m_meshIndex, m_pivot,
      [this, &skinMatrixCount](const Mesh& mesh, const Matrix4& restMatrix, uint32 node) {
        MeshPart& part = m_parts.emplace_back();
        part.localMatrix = node != ALL && isAnimating() ? m_poseMatrices[node] : restMatrix;
        part.mesh = &mesh;
        part.node = node;
        // Only a whole model has the node matrices the bones need.
        if (node != ALL && mesh.hasSkin()) {
          part.firstSkinMatrix = skinMatrixCount;
          skinMatrixCount += static_cast<uint32>(mesh.getSkin().boneNodes.size());
        }
      });
  m_skinMatrices.resize(skinMatrixCount);

  Scene& scene = *getOwner()->getScene();
  for (MeshPart& part : m_parts) {
    const RenderItem item{
        .mesh = part.mesh,
        .material = getRenderMaterial(part.mesh->getMaterialSlot()),
        .owner = getOwner(),
        .skinMatrices =
            part.firstSkinMatrix == ALL ? nullptr : &m_skinMatrices[part.firstSkinMatrix]};
    part.renderItemId = scene.addRenderItem(item);
  }
  updateSkinMatrices();
  updateRenderItems();
}

/*
 */
void
ModelComponent::removeRenderItems()
{
  Scene& scene = *getOwner()->getScene();
  for (const MeshPart& part : m_parts) {
    scene.removeRenderItem(part.renderItemId);
  }
  m_parts.clear();
  m_skinMatrices.clear();
}

} // namespace chEngineSDK
