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

// Only the files that allocate include D3D12MemAlloc.h.
namespace D3D12MA {
class Allocator;
class Allocation;
} // namespace D3D12MA

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

/**
 * Format of the resource of a depth texture that shaders also read: typeless, so it takes
 * both a depth target view and a shader view.
 */
NODISCARD FORCEINLINE DXGI_FORMAT
getTypelessDepthFormat(Format format)
{
  switch (format) {
  case Format::D24_UNORM_S8_UINT:
    return DXGI_FORMAT_R24G8_TYPELESS;
  case Format::D32_SFLOAT_S8_UINT:
    return DXGI_FORMAT_R32G8X24_TYPELESS;
  case Format::D32_SFLOAT:
  default:
    return DXGI_FORMAT_R32_TYPELESS;
  }
}

/**
 * Format shaders read the depth of a depth texture with.
 */
NODISCARD FORCEINLINE DXGI_FORMAT
getDepthShaderFormat(Format format)
{
  switch (format) {
  case Format::D24_UNORM_S8_UINT:
    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
  case Format::D32_SFLOAT_S8_UINT:
    return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
  case Format::D32_SFLOAT:
  default:
    return DXGI_FORMAT_R32_FLOAT;
  }
}

NODISCARD FORCEINLINE DXGI_FORMAT
chVertexFormatToDxgiFormat(VertexFormat format)
{
  // In the order of the VertexFormat values.
  static constexpr DXGI_FORMAT VERTEX_FORMATS[] = {
      DXGI_FORMAT_R32_FLOAT,          // Float
      DXGI_FORMAT_R32G32_FLOAT,       // Float2
      DXGI_FORMAT_R32G32B32_FLOAT,    // Float3
      DXGI_FORMAT_R32G32B32A32_FLOAT, // Float4
      DXGI_FORMAT_R32_SINT,           // Int
      DXGI_FORMAT_R32G32_SINT,        // Int2
      DXGI_FORMAT_R32G32B32_SINT,     // Int3
      DXGI_FORMAT_R32G32B32A32_SINT,  // Int4
      DXGI_FORMAT_R32_UINT,           // UInt
      DXGI_FORMAT_R32G32_UINT,        // UInt2
      DXGI_FORMAT_R32G32B32_UINT,     // UInt3
      DXGI_FORMAT_R32G32B32A32_UINT,  // UInt4
      DXGI_FORMAT_R8G8B8A8_SINT,      // Byte4
      DXGI_FORMAT_R8G8B8A8_SNORM,     // Byte4Normalized
      DXGI_FORMAT_R8G8B8A8_UINT,      // UByte4
      DXGI_FORMAT_R8G8B8A8_UNORM,     // UByte4Normalized
      DXGI_FORMAT_R16G16_SINT,        // Short2
      DXGI_FORMAT_R16G16_SNORM,       // Short2Normalized
      DXGI_FORMAT_R16G16B16A16_SINT,  // Short4
      DXGI_FORMAT_R16G16B16A16_SNORM, // Short4Normalized
  };
  static_assert(std::size(VERTEX_FORMATS) == static_cast<SIZE_T>(VertexFormat::COUNT),
                "Every VertexFormat needs its DXGI format.");
  return VERTEX_FORMATS[static_cast<uint32>(format)];
}

NODISCARD FORCEINLINE D3D12_COMPARISON_FUNC
chCompareOpToD3D12(CompareOp compareOp)
{
  switch (compareOp) {
  case CompareOp::Never:
    return D3D12_COMPARISON_FUNC_NEVER;
  case CompareOp::Less:
    return D3D12_COMPARISON_FUNC_LESS;
  case CompareOp::Equal:
    return D3D12_COMPARISON_FUNC_EQUAL;
  case CompareOp::LessOrEqual:
    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
  case CompareOp::Greater:
    return D3D12_COMPARISON_FUNC_GREATER;
  case CompareOp::NotEqual:
    return D3D12_COMPARISON_FUNC_NOT_EQUAL;
  case CompareOp::GreaterOrEqual:
    return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
  case CompareOp::AlwaysOp:
  default:
    return D3D12_COMPARISON_FUNC_ALWAYS;
  }
}

} // namespace chEngineSDK

#define DX12_CHECK(result) chEngineSDK::throwIfFailed(result, __FILE__, __LINE__)
