/************************************************************************/
/**
 * @file chPrerequisitesDX12.h
 * @author AccelMR
 * @date 2022/09/14
 *    Wrapper around the DirectX 12 render API.
 *
 * Wrapper around the DirectX 12 render API.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include <chPrerequisitesCore.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <chStringID.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <d3d12sdklayers.h>

#include "chForwardDecDX12.h"

template<typename T>
void SafeRelease(T * &p) {
  if (p) {
    p->Release();
    p = nullptr;
  }
}
namespace chEngineSDK{

// const SIZE_T TEXTURE_ALLOC_ID = StringID("TextureAlloc").getId();
// const SIZE_T COMMAND_LIST_ALLOC_ID = StringID("CommandListAlloc").getId();
// const SIZE_T CONSTANT_BUFFER_ALLOC_ID = StringID("ConstantBufferAlloc").getId();

/*
*/
inline void ThrowIfFailed(HRESULT hr) {
  if (FAILED(hr)) {
    String message = std::system_category().message(hr);
    CH_EXCEPT(InternalErrorException, "HR failed in DX12 module." + message);
  }
}
}