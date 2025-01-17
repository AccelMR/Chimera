/************************************************************************/
/**
 * @file chPrerequisitesDX12.h
 * @author AccelMR
 * @date 2023/02/20
 *   Must basic include for every dx12 file
 */
 /************************************************************************/
#include "chPrerequisitesCore.h"

#include "Win32/chWindows.h"

#include <d3d12.h>
#include <dxgi1_6.h>

namespace chDX12EngineSDK {
inline void ThrowIfFailed(HRESULT hr)
{
#if USING(CH_DEBUG_MODE)
  if (FAILED(hr)) { throw std::exception("HRESULT error code"); }
#endif
}
}
