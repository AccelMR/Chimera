/************************************************************************/
/**
 * @file chModelComponent.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  Component that draws a Model, or one part of it, at its GameObject.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAnimationClip.h"
#include "chBox.h"
#include "chComponent.h"
#include "chMatrix4.h"
#include "chTypeTraits.h"
#include "chVector3.h"

namespace chEngineSDK {
class AnimationClipAsset;
class Material;
class MaterialAsset;
class ModelAsset;

/**
 * Exists to put a Model in a scene. It draws the whole model, the meshes of one node or a
 * single mesh, so a model can be split into objects that move on their own. While
 * registered it adds one render item per mesh it draws to the scene, and moves them when
 * the GameObject moves, so the renderer never looks at components.
 *
 * A whole model can also play an animation clip: update() moves the nodes the clip's
 * tracks name (matched by name when the clip is set) and the render items of their meshes.
 * The pose lives here, never in GameObjects, so bones stay out of the scene graph.
 */
class CH_CORE_EXPORT ModelComponent : public Component
{
 public:
  ModelComponent() = default;

  explicit ModelComponent(const SPtr<ModelAsset>& modelAsset);

  /**
   * Draws one node (an index of Model::getNodes) and one of its meshes, or all of them
   * with ALL. The object stands at pivot, a point in the node's space, so the meshes are
   * drawn moved back by it.
   */
  ModelComponent(const SPtr<ModelAsset>& modelAsset,
                 uint32 nodeIndex,
                 uint32 meshIndex,
                 const Vector3& pivot);

  /**
   * Draws the whole model and forgets every override.
   */
  void
  setModel(const SPtr<ModelAsset>& modelAsset);

  /**
   * Same as the constructor with a part; forgets every override.
   */
  void
  setModelPart(const SPtr<ModelAsset>& modelAsset,
               uint32 nodeIndex,
               uint32 meshIndex,
               const Vector3& pivot);

  NODISCARD FORCEINLINE const SPtr<ModelAsset>&
  getModelAsset() const { return m_modelAsset; }

  /**
   * The model of the asset, kept while the component uses it, even if the asset unloads.
   */
  NODISCARD FORCEINLINE const SPtr<Model>&
  getModel() const { return m_model; }

  NODISCARD FORCEINLINE bool
  isWholeModel() const { return m_nodeIndex == ALL; }

  NODISCARD FORCEINLINE uint32
  getNodeIndex() const { return m_nodeIndex; }

  NODISCARD FORCEINLINE uint32
  getMeshIndex() const { return m_meshIndex; }

  NODISCARD FORCEINLINE const Vector3&
  getPivot() const { return m_pivot; }

  /**
   * The model's material slots the drawn meshes use, in increasing order.
   */
  NODISCARD FORCEINLINE const Vector<uint32>&
  getUsedMaterialSlots() const { return m_usedSlots; }

  /**
   * Replaces the model's material of that slot on this object only; null goes back to it.
   * Ignored for a slot the drawn meshes do not use.
   */
  void
  setMaterial(uint32 slot, const SPtr<MaterialAsset>& material);

  /**
   * Null when the slot uses the model's material.
   */
  NODISCARD const SPtr<MaterialAsset>&
  getMaterialOverride(uint32 slot) const;

  /**
   * The override, else the model's material; null when neither exists.
   */
  NODISCARD const SPtr<MaterialAsset>&
  getMaterial(uint32 slot) const;

  /**
   * Plays clip from its start (null stops animating and goes back to the rest pose). Only a
   * whole model is animated; a part keeps the clip but stands still.
   */
  void
  setAnimation(const SPtr<AnimationClipAsset>& clip);

  NODISCARD FORCEINLINE const SPtr<AnimationClipAsset>&
  getAnimationAsset() const { return m_animationAsset; }

  /**
   * The clip of the asset, kept while the component uses it, even if the asset unloads.
   */
  NODISCARD FORCEINLINE const SPtr<AnimationClip>&
  getAnimation() const { return m_animation; }

  /**
   * True when the clip moves at least one node of the model.
   */
  NODISCARD FORCEINLINE bool
  isAnimating() const { return !m_poseMatrices.empty(); }

  /**
   * Playing a clip that does not loop and already reached its end starts it again.
   */
  void
  setPlaying(bool bPlaying);

  NODISCARD FORCEINLINE bool
  isPlaying() const { return m_bPlaying; }

  void
  setLooping(bool bLooping) { m_bLooping = bLooping; }

  NODISCARD FORCEINLINE bool
  isLooping() const { return m_bLooping; }

  /**
   * Seconds of the clip per second of play; negative plays backwards.
   */
  void
  setPlaybackSpeed(float speed) { m_playbackSpeed = speed; }

  NODISCARD FORCEINLINE float
  getPlaybackSpeed() const { return m_playbackSpeed; }

  /**
   * Jumps to a time of the clip (clamped to it); the pose follows on the next update.
   */
  void
  setAnimationTime(float time);

  NODISCARD FORCEINLINE float
  getAnimationTime() const { return m_animationTime; }

  /**
   * Where the node stands in the model's space in the current pose: the rest pose when
   * nothing is animated.
   */
  NODISCARD const Matrix4&
  getNodeMatrix(uint32 nodeIndex) const;

  void
  update(float deltaTime) override;

  NODISCARD bool
  getWorldBounds(AABox& outBounds) const override;

  void
  serialize(DataStream& stream) const override;

  NODISCARD bool
  deserialize(DataStream& stream) override;

  void
  collectReferences(Vector<UUID>& outReferences) const override;

  static constexpr uint32 ALL = 0xFFFFFFFFu;
  static constexpr uint32 SERIALIZATION_VERSION = 2;

 protected:
  void
  onRegister() override;

  void
  onUnregister() override;

  void
  onTransformChanged() override;

 private:
  struct MeshPart
  {
    // Where the mesh sits relative to the GameObject.
    Matrix4 localMatrix;
    const Mesh* mesh = nullptr;
    uint32 renderItemId = 0;
    // The model node that draws the mesh, or ALL for a part (its node is the GameObject).
    uint32 node = ALL;
  };

  /**
   * Finds the node of each track of the clip and starts the pose at rest; leaves nothing to
   * animate when the clip moves no node of a whole model.
   */
  void
  bindAnimation();

  /**
   * Samples the clip at the current time and rebuilds the pose matrices.
   */
  void
  computePose();

  /**
   * Gives each part drawn by a node its matrix in the current pose, then moves the render
   * items.
   */
  void
  refreshPartMatrices();

  /**
   * Writes the world matrix and bounds of each render item from its part's local matrix.
   */
  void
  updateRenderItems();

  void
  rebuildUsedSlots();

  void
  addRenderItems();

  void
  removeRenderItems();

  NODISCARD const Material*
  getRenderMaterial(uint32 slot) const;

  /**
   * Index in m_usedSlots, or ALL when the slot is not used.
   */
  NODISCARD uint32
  findUsedSlot(uint32 slot) const;

  SPtr<ModelAsset> m_modelAsset;
  SPtr<Model> m_model;
  // Kept when the asset is missing, so saving again keeps the reference.
  UUID m_modelAssetId = UUID::null();
  uint32 m_nodeIndex = ALL;
  uint32 m_meshIndex = ALL;
  Vector3 m_pivot = Vector3::ZERO;
  Vector<uint32> m_usedSlots;
  // One per used slot; null uses the model's material.
  Vector<SPtr<MaterialAsset>> m_materialOverrides;
  Vector<MeshPart> m_parts;

  SPtr<AnimationClipAsset> m_animationAsset;
  SPtr<AnimationClip> m_animation;
  // Kept when the asset is missing, so saving again keeps the reference.
  UUID m_animationAssetId = UUID::null();
  float m_animationTime = 0.0f;
  float m_playbackSpeed = 1.0f;
  bool m_bPlaying = true;
  bool m_bLooping = true;
  // The time changed without the pose following yet.
  bool m_bPoseDirty = false;
  // Per track of the clip, the model node it moves, or AnimationClip::NO_TARGET.
  Vector<uint32> m_trackNodes;
  // Per model node, only while a clip moves the model: where it stands relative to its
  // parent, and in the model's space.
  Vector<NodeTransform> m_localPose;
  Vector<Matrix4> m_poseMatrices;
};
DECLARE_TYPE_TRAITS(ModelComponent)

} // namespace chEngineSDK
