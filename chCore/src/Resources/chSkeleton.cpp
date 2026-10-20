/************************************************************************/
/**
 * @file chSkeleton.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Bone hierarchy shared by the models and animations made for it.
 */
/************************************************************************/
#include "chSkeleton.h"

#include "chAssetFile.h"
#include "chFileStream.h"

namespace chEngineSDK {
namespace {
// Above any real skeleton; only a broken file asks for more.
constexpr uint32 kMaxBones = 65536;
constexpr uint32 kMaxNameLength = 1024;

struct BoneFileData
{
  uint32 parent;
  float position[3];
  float rotation[4];
  float scale[3];
};
static_assert(sizeof(BoneFileData) == 44, "BoneFileData must have no padding");
} // namespace

/*
 */
uint32
Skeleton::addBone(SkeletonBone bone)
{
  const uint32 index = getBoneCount();
  CH_ASSERT(bone.parent == SkeletonBone::NO_PARENT || bone.parent < index);
  if (bone.parent != SkeletonBone::NO_PARENT && bone.parent >= index) {
    bone.parent = SkeletonBone::NO_PARENT;
  }
  m_bones.push_back(std::move(bone));
  return index;
}

/*
 */
uint32
Skeleton::findBone(StringView name) const
{
  for (uint32 i = 0; i < m_bones.size(); ++i) {
    if (m_bones[i].name == name) {
      return i;
    }
  }
  return NO_BONE;
}

/*
 */
bool
Skeleton::mapBones(const Skeleton& other, Vector<uint32>& outBoneMap) const
{
  outBoneMap.assign(other.getBoneCount(), NO_BONE);
  for (uint32 i = 0; i < other.getBoneCount(); ++i) {
    const SkeletonBone& bone = other.m_bones[i];
    const uint32 mapped = findBone(bone.name);
    if (mapped == NO_BONE) {
      return false;
    }
    // Parents come first, so the parent is already mapped. A root of other may hang from a
    // bone here that other lacks.
    if (bone.parent != SkeletonBone::NO_PARENT &&
        m_bones[mapped].parent != outBoneMap[bone.parent]) {
      return false;
    }
    outBoneMap[i] = mapped;
  }
  return true;
}

/*
 */
void
Skeleton::serialize(DataStream& stream) const
{
  stream << SERIALIZATION_VERSION;
  stream << getBoneCount();
  for (const SkeletonBone& bone : m_bones) {
    const BoneFileData data{
        .parent = bone.parent,
        .position = {bone.position.x, bone.position.y, bone.position.z},
        .rotation = {bone.rotation.x, bone.rotation.y, bone.rotation.z, bone.rotation.w},
        .scale = {bone.scale.x, bone.scale.y, bone.scale.z}};
    AssetFile::writeString(stream, bone.name);
    stream << data;
  }
}

/*
 */
SPtr<Skeleton>
Skeleton::deserialize(DataStream& stream)
{
  uint32 version = 0;
  uint32 boneCount = 0;
  if (!AssetFile::readValue(stream, version) || version != SERIALIZATION_VERSION ||
      !AssetFile::readValue(stream, boneCount) || boneCount > kMaxBones) {
    return nullptr;
  }

  SPtr<Skeleton> skeleton = chMakeShared<Skeleton>();
  for (uint32 i = 0; i < boneCount; ++i) {
    SkeletonBone bone;
    BoneFileData data;
    if (!AssetFile::readString(stream, bone.name, kMaxNameLength) ||
        !AssetFile::readValue(stream, data) ||
        (data.parent != SkeletonBone::NO_PARENT && data.parent >= i)) {
      return nullptr;
    }
    bone.parent = data.parent;
    bone.position = Vector3(data.position[0], data.position[1], data.position[2]);
    bone.rotation =
        Quaternion(data.rotation[0], data.rotation[1], data.rotation[2], data.rotation[3]);
    bone.scale = Vector3(data.scale[0], data.scale[1], data.scale[2]);
    skeleton->addBone(std::move(bone));
  }
  return skeleton;
}

} // namespace chEngineSDK
