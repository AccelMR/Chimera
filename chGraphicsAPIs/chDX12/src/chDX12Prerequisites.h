/************************************************************************/
/**
 * @file chDX12Prerequisites.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 headers, error checks and format conversions shared by the plugin.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chException.h"
#include "chGraphicsTypes.h"
#include "chLogger.h"
#include "chStringUtils.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace chEngineSDK {
CH_LOG_DECLARE_EXTERN(, DX12);

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

/**
 * Output argument of the calls that create an object, given with the IID_ constants of
 * dxguid; IID_PPV_ARGS relies on the __uuidof compiler extension.
 */
template<typename T>
NODISCARD FORCEINLINE void**
outPtr(ComPtr<T>& pointer)
{
  return reinterpret_cast<void**>(pointer.ReleaseAndGetAddressOf());
}

class DX12ErrorException : public Exception
{
 public:
  DX12ErrorException(const String& inDescription,
                     const String& inSource,
                     const ANSICHAR* inFile,
                     uint32 inLine)
    : Exception("DX12ErrorException", inDescription, inSource, inFile, inLine)
  {}
};

FORCEINLINE void
throwIfFailed(HRESULT result, const ANSICHAR* file, uint32 line)
{
  if (FAILED(result)) {
    CH_EXCEPT(DX12ErrorException,
              StringUtils::format("Direct3D 12 error 0x{0:X} at {1}:{2}",
                                  static_cast<uint32>(result), file, line));
  }
}

NODISCARD FORCEINLINE DXGI_FORMAT
chFormatToDxgiFormat(Format format)
{
  // In the order of the Format values, so a conversion is one array read.
  static constexpr DXGI_FORMAT DXGI_FORMATS[] = {
      DXGI_FORMAT_UNKNOWN,              // Unknown
      DXGI_FORMAT_R8G8B8A8_UNORM,       // R8G8B8A8_UNORM
      DXGI_FORMAT_B8G8R8A8_UNORM,       // B8G8R8A8_UNORM
      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,  // B8G8R8A8_SRGB
      DXGI_FORMAT_R16G16B16A16_FLOAT,   // R16G16B16A16_SFLOAT
      DXGI_FORMAT_D32_FLOAT,            // D32_SFLOAT
      DXGI_FORMAT_D24_UNORM_S8_UINT,    // D24_UNORM_S8_UINT
      DXGI_FORMAT_R8_UNORM,             // R8_UNORM
      DXGI_FORMAT_R8G8_UNORM,           // R8G8_UNORM
      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  // R8G8B8A8_SRGB
      DXGI_FORMAT_R10G10B10A2_UNORM,    // A2B10G10R10_UNORM
      DXGI_FORMAT_R11G11B10_FLOAT,      // B10G11R11_UFLOAT
      DXGI_FORMAT_R16_FLOAT,            // R16_SFLOAT
      DXGI_FORMAT_R16G16_FLOAT,         // R16G16_SFLOAT
      DXGI_FORMAT_R32_FLOAT,            // R32_SFLOAT
      DXGI_FORMAT_R32G32B32A32_FLOAT,   // R32G32B32A32_SFLOAT
      DXGI_FORMAT_R32_UINT,             // R32_UINT
      DXGI_FORMAT_D32_FLOAT_S8X24_UINT, // D32_SFLOAT_S8_UINT
  };
  static_assert(std::size(DXGI_FORMATS) == static_cast<SIZE_T>(Format::COUNT),
                "Every Format needs its DXGI format.");
  return DXGI_FORMATS[static_cast<uint32>(format)];
}

} // namespace chEngineSDK

#define DX12_CHECK(result) chEngineSDK::throwIfFailed(result, __FILE__, __LINE__)
