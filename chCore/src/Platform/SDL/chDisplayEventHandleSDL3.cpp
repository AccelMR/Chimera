/************************************************************************/
/**
 * @file chDisplayEventHandleSDL3.cpp
 * @author AccelMR
 * @date 2025/07/07
 * @brief
 * SDL3 specific implementation of DisplayEventHandle.
 */
/************************************************************************/
#include "chDisplayEventHandle.h"

#if USING (CH_DISPLAY_SDL3)

namespace chEngineSDK {

/*
*/
DisplayEventHandle::DisplayEventHandle(uint32 width, uint32 height) :
  m_previousWidth(width),
  m_previousHeight(height) {}

/*
*/
DisplayEventHandle::~DisplayEventHandle() {
}

/*
*/
void
DisplayEventHandle::update() {
}

}  // namespace chEngineSDK

#endif // USING(CH_DISPLAY_SDL3)
