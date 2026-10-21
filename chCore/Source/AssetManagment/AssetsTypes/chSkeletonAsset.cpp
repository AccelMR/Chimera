/************************************************************************/
/**
 * @file chSkeletonAsset.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Asset that holds a Skeleton.
 */
/************************************************************************/
#include "chSkeletonAsset.h"

#include "chFileStream.h"
#include "chLogger.h"
#include "chSkeleton.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(SkeletonAssetLog, All);

/*
 */
bool
SkeletonAsset::serialize(SPtr<DataStream> stream)
{
  if (!stream || !m_skeleton) {
    CH_LOG(SkeletonAssetLog, Error, "SkeletonAsset {0} has no skeleton to save",
           m_metadata.name);
    return false;
  }
  m_skeleton->serialize(*stream);
  return true;
}

/*
 */
bool
SkeletonAsset::deserialize(SPtr<DataStream> stream)
{
  m_skeleton = stream ? Skeleton::deserialize(*stream) : nullptr;
  if (!m_skeleton) {
    CH_LOG(SkeletonAssetLog, Error, "SkeletonAsset {0} could not be read", m_metadata.name);
    return false;
  }
  return true;
}

/*
 */
void
SkeletonAsset::clearAssetData()
{
  m_skeleton.reset();
}

} // namespace chEngineSDK
