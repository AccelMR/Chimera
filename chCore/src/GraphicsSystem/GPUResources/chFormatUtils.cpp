/************************************************************************/
/**
 * @file chTextureFormats.cpp
 * @author AccelMR
 * @date 2024/12/20
 *    All available formats in the engine available
 */
 /************************************************************************/

/************************************************************************/
/*
  * Includes
  */
/************************************************************************/
#include "chFormats.h"

namespace chEngineSDK{
namespace chFormatUtils{
/*
*/
uint32
getFormatSize(const FORMAT format) {
  switch (format) {
    case FORMAT::kR32G32B32A32_TYPELESS:
    case FORMAT::kR32G32B32A32_FLOAT:
    case FORMAT::kR32G32B32A32_UINT:
    case FORMAT::kR32G32B32A32_SINT:
      return 16;
    case FORMAT::kR32G32B32_TYPELESS:
    case FORMAT::kR32G32B32_FLOAT:
    case FORMAT::kR32G32B32_UINT:
    case FORMAT::kR32G32B32_SINT:
      return 12;
    case FORMAT::kR16G16B16A16_TYPELESS:
    case FORMAT::kR16G16B16A16_FLOAT:
    case FORMAT::kR16G16B16A16_UNORM:
    case FORMAT::kR16G16B16A16_UINT:
    case FORMAT::kR16G16B16A16_SNORM:
    case FORMAT::kR16G16B16A16_SINT:
      return 8;
    case FORMAT::kR32G32_TYPELESS:
    case FORMAT::kR32G32_FLOAT:
    case FORMAT::kR32G32_UINT:
    case FORMAT::kR32G32_SINT:
      return 8;
    case FORMAT::kR32G8X24_TYPELESS:
    case FORMAT::kD32_FLOAT_S8X24_UINT:
    case FORMAT::kR32_FLOAT_X8X24_TYPELESS:
    case FORMAT::kX32_TYPELESS_G8X24_UINT:
      return 8;
    case FORMAT::kR10G10B10A2_TYPELESS:
    case FORMAT::kR10G10B10A2_UNORM:
    case FORMAT::kR10G10B10A2_UINT:
      return 4;
    case FORMAT::kR11G11B10_FLOAT:
      return 4;
    case FORMAT::kR8G8B8A8_TYPELESS:
    case FORMAT::kR8G8B8A8_UNORM:
    case FORMAT::kR8G8B8A8_UNORM_SRGB:
    case FORMAT::kR8G8B8A8_UINT:
    case FORMAT::kR8G8B8A8_SNORM:
    case FORMAT::kR8G8B8A8_SINT:
      return 4;
    case FORMAT::kR16G16_TYPELESS:
    case FORMAT::kR16G16_FLOAT:
    case FORMAT::kR16G16_UNORM:
    case FORMAT::kR16G16_UINT:
    case FORMAT::kR16G16_SNORM:
    case FORMAT::kR16G16_SINT:
      return 4;
    default:
      return 0;
  }
}

} // namespace chFormatUtils
} // namespace chEngineSDK