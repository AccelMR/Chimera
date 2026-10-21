/************************************************************************/
/**
 * @file chSkeleton.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Bone hierarchy shared by the models and animations made for it.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chQuaternion.h"
#include "chVector3.h"

namespace chEngineSDK {
class DataStream;

/**
 * One bone: its name, its parent and its rest transform relative to the parent.
 */
struct SkeletonBone
{
  static constexpr uint32 NO_PARENT = 0xFFFFFFFFu;

  String name;
  // Lower than the bone's own index, so a parent always comes before its children.
  uint32 parent = NO_PARENT;
  Vector3 position = Vector3::ZERO;
  Quaternion rotation = Quaternion::IDENTITY;
  Vector3 scale = Vector3::UNIT;
};

/**
 * Exists so models and animations that use the same bones can share them: animations
 * target bones by index here, and every model says which of its nodes is which bone. Bones
 * are a flat array with parents first, like the nodes of a Model.
 */
class CH_CORE_EXPORT Skeleton
{
 public:
  /**
   * The parent must be NO_PARENT or a bone added before. Returns the index of the bone.
   */
  uint32
  addBone(SkeletonBone bone);

  NODISCARD FORCEINLINE const Vector<SkeletonBone>&
  getBones() const noexcept { return m_bones; }

  NODISCARD FORCEINLINE uint32
  getBoneCount() const noexcept { return static_cast<uint32>(m_bones.size()); }

  /**
   * Index of the bone with that name, or NO_BONE. Walks every bone, so it is meant for
   * import and load, not for every frame.
   */
  NODISCARD uint32
  findBone(StringView name) const;

  /**
   * True when every bone of other is here with the same name and its parent is here too,
   * as the parent of that bone. outBoneMap gets, per bone of other, the index here. This
   * skeleton may have bones that other lacks.
   */
  NODISCARD bool
  mapBones(const Skeleton& other, Vector<uint32>& outBoneMap) const;

  void
  serialize(DataStream& stream) const;

  /**
   * Null when the data is broken.
   */
  NODISCARD static SPtr<Skeleton>
  deserialize(DataStream& stream);

  static constexpr uint32 NO_BONE = 0xFFFFFFFFu;
  static constexpr uint32 SERIALIZATION_VERSION = 1;

 private:
  Vector<SkeletonBone> m_bones;
};

} // namespace chEngineSDK
