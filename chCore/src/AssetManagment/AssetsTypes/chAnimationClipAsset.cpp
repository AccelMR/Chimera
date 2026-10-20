/************************************************************************/
/**
 * @file chAnimationClipAsset.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Asset that holds an AnimationClip.
 */
/************************************************************************/
#include "chAnimationClipAsset.h"

#include "chFileStream.h"
#include "chLogger.h"
#include "chAnimationClip.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(AnimationClipAssetLog, All);

/*
 */
bool
AnimationClipAsset::serialize(SPtr<DataStream> stream)
{
  if (!stream || !m_clip) {
    CH_LOG(AnimationClipAssetLog, Error, "AnimationClipAsset {0} has no clip to save",
           m_metadata.name);
    return false;
  }
  m_clip->serialize(*stream);
  return true;
}

/*
 */
bool
AnimationClipAsset::deserialize(SPtr<DataStream> stream)
{
  m_clip = stream ? AnimationClip::deserialize(*stream) : nullptr;
  if (!m_clip) {
    CH_LOG(AnimationClipAssetLog, Error, "AnimationClipAsset {0} could not be read", m_metadata.name);
    return false;
  }
  return true;
}

/*
 */
void
AnimationClipAsset::clearAssetData()
{
  m_clip.reset();
}

} // namespace chEngineSDK
