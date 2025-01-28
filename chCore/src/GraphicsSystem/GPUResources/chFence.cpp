/************************************************************************/
/**
 * @file chFence.CPP
 * @author AccelMR
 * @date 2025/01/27
 * 
 *  Fence GPU interface
 *
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chFence.h"

namespace chEngineSDK {
/*
*/
void
Fence::wait(uint64 timeout) {
  _internalWait(timeout);
}

/*
*/
void
Fence::reset() {
  _internalReset();
}
} // namespace chEngineSDK
