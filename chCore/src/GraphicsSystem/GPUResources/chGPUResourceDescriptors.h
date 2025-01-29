/************************************************************************/
/**
 * @file chResourceDescriptors.h
 * @author AccelMR
 * @date 2022/09/23
 *   Here goes all the descriptor related with graphics and resources.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"
#include "chPrerequisitesGraphics.h"
#include "chLinearColor.h"
#include "chBox2D.h"
#include "chFlags.h"
#include "chVector3I.h"
#include "chFileStream.h"
#include "chStringID.h"

#include "chFormats.h"

namespace chEngineSDK{
namespace chGPUDesc {
/** 
 *   Do not use this as TEXTURE_USAGE since it's a flag. Use TextUsageFlag
 **/
 enum class TEXTURE_USAGE : uint32 {
   kUSAGE_SAMPLED = 0x1,
   kUSAGE_STORAGE = 0x2,
   kUSAGE_DEPTH_STENCIL = 0x4,
   kUSAGE_RENDER_TARGET = 0x8,
   kUSAGE_RENDER_TARGET_READ = 0x10,
 };
 CH_FLAGS_OPERATORS_EXT(TEXTURE_USAGE, uint32);
 using TextUsageFlag = Flags<TEXTURE_USAGE, uint32>;

/*
 * Multrisample count, used for sample in pipelines
*/
enum class SAMPLE_COUNT : uint32 {
  kSAMPLE_COUNT_1 = 1,
  kSAMPLE_COUNT_2 = 2,
  kSAMPLE_COUNT_4 = 4,
  kSAMPLE_COUNT_8 = 8,
  kSAMPLE_COUNT_16 = 16,
  kSAMPLE_COUNT_32 = 32,
  kSAMPLE_COUNT_64 = 64
};
CH_FLAGS_OPERATORS_EXT(SAMPLE_COUNT, uint32);
using SampleCountFlag = Flags<SAMPLE_COUNT, uint32>;

 /** 
 *   Do not use this as SHADER_STAGE since it's a flag. Use ShaderStageFlag
 **/
 enum class SHADER_STAGE : uint16 {
   kVERTEX = 0x1,
   kPIXEL = 0x2,
   kCOMPUTE = 0x4,
   kMESH = 0x8,
   ALL = kVERTEX | kPIXEL | kCOMPUTE | kMESH
 };
 CH_FLAGS_OPERATORS_EXT(SHADER_STAGE, uint16);
 using ShaderStageFlag = Flags<SHADER_STAGE, uint16>;

 enum class BARRIER_FLAG : uint32 {
   kNONE = 0,
   kBEGIN_ONLY = 0x1,
   kEND_ONLY = 0x2
 };
 CH_FLAGS_OPERATORS_EXT(BARRIER_FLAG, uint32);
 using BarrierFlag = Flags<BARRIER_FLAG, uint32>;

 enum class COLOR_WRITE_ENABLE: uint8 {
   kCOLOR_WRITE_ENABLE_RED  = 1,
   kCOLOR_WRITE_ENABLE_GREEN = 2,
   kCOLOR_WRITE_ENABLE_BLUE = 4,
   kCOLOR_WRITE_ENABLE_ALPHA = 8,
   kCOLOR_WRITE_ENABLE_ALL = 15
 };
 CH_FLAGS_OPERATORS_EXT(COLOR_WRITE_ENABLE, uint8);
 using ColorWriteEnableFlag = Flags<COLOR_WRITE_ENABLE, uint8>;

 /** 
  *   Texture descriptor.
  **/
 struct TextureDesc {
    enum class TYPE {
      kTYPE_1D,
      kTYPE_2D,
      kTYPE_3D
    };
    static constexpr uint32 ALL_MIPS = 0xffffffff;

    TYPE type = TYPE::kTYPE_2D;
    Vector3I dimensions = Vector3I::UNIT;
    uint32 mips = ALL_MIPS;
    uint32 layers = 1;
    TextUsageFlag usage = TEXTURE_USAGE::kUSAGE_SAMPLED;
    FORMAT format = FORMAT::kR8G8B8A8_UNORM;
  };

/** 
 *   Describes how a texture will be used for shaders.
 **/
 struct TextureBindingDesc {
   enum class TYPE {
     kSAMPLED,
     kSTORAGE,
     kRENDER_TARGET_READ
   };

   TYPE type = TYPE::kSAMPLED;
   ShaderStageFlag stages = SHADER_STAGE::ALL;
   uint32 slot = 0;

   Vector<SPtr<Texture>> textures;
   Vector<SPtr<Sampler>> samplers;
 };

 /** 
  *   Describes how a buffer will be used for shaders.
  **/
 struct BufferBindingDesc {
  enum class TYPE {
    kUNIFORM,
    kSTORAGE
  };
  BufferBindingDesc(TYPE _type, 
                    ShaderStageFlag _stages,
                    uint32 _slot, 
                    uint32 _size,
                    SPtr<GPUBuffer> _buffer) :
    type(_type),
    stages(_stages),
    slot(_slot),
    size(_size),
    buffer(_buffer) {}

  TYPE type = TYPE::kUNIFORM;
  ShaderStageFlag stages = SHADER_STAGE::ALL;
  uint32 slot = 0;
  uint32 size;
  SPtr<GPUBuffer> buffer;
 };

 /** 
  *   Describes the bindings for a shader or group of shader.
  **/
 struct BindingGroup {
   uint32 slot = 0;
   Vector<TextureBindingDesc> textures;
   Vector<BufferBindingDesc> buffers;
 };

 /** 
  *   Shader inner informations to fill.
  **/
 struct ShaderInfo {
   Vector<uint8> byteCode;
   String entryFunc = "main";
 };
 
 enum class VERTEX_SEMANTIC {
   kPOSITION,
   kBINORMAL,
   kTANGENT,
   kTEXTCOORD,
   kNORMAL,
   kCOLOR
 };

 struct VertexBufferBindingDesc {
  VertexBufferBindingDesc(VERTEX_SEMANTIC _vertexSemantic, 
                          uint32 _slot, 
                          uint32 _byteStride, 
                          FORMAT _format) :
    vertexSemantic(_vertexSemantic),
    slot(_slot),
    byteStride(_byteStride),
    format(_format) {}
    
  VERTEX_SEMANTIC vertexSemantic; //optional depending on api?
  uint32 slot;
  uint32 byteStride; //Aligned offset
  FORMAT format;
 };

/** 
 *   Describes how a shader will be created and its state, etc.
 **/
struct ShaderDesc {
  String name = "Undefined";
  Vector<uint8> byteCode;
  String entryFunc = "main";
};

 enum class FILL_MODE {
   kWIREFRAME,
   kSOLID
 };

 enum class CULL_MODE {
   kNONE = 1,
   kFRONT = 2,
   kBACK = 3
 };

 struct RasterizerStateDesc {
  FILL_MODE fillMode = FILL_MODE::kSOLID;
  CULL_MODE cullMode = CULL_MODE::kBACK;
  bool frontCounterClockWise = true;
  int32 depthBias = 0;
  float depthBiasClamp = 0.0f;
  float slopeScaledDepthBias = 0.0f;
  bool depthClipEnable = true;
  bool multiSampleEnable = false;
  bool antialiasedLineEnable = false;
  uint32 forcedSampleCount = 0;
  float lineWidth = 1.0f;
 };

 struct RenderTargetBlendState {
  bool blenEnable = false;
  bool logicOpEnable = false;

 };

 enum class BLEND {
   kBLEND_ZERO = 1,
   kBLEND_ONE = 2,
   kBLEND_SRC_COLOR = 3,
   kBLEND_INV_SRC_COLOR = 4,
   kBLEND_SRC_ALPHA = 5,
   kBLEND_INV_SRC_ALPHA = 6,
   kBLEND_DEST_ALPHA = 7,
   kBLEND_INV_DEST_ALPHA = 8,
   kBLEND_DEST_COLOR = 9,
   kBLEND_INV_DEST_COLOR = 10,
   kBLEND_SRC_ALPHA_SAT = 11,
   kBLEND_BLEND_FACTOR = 14,
   kBLEND_INV_BLEND_FACTOR = 15,
   kBLEND_SRC1_COLOR = 16,
   kBLEND_INV_SRC1_COLOR = 17,
   kBLEND_SRC1_ALPHA = 18,
   kBLEND_INV_SRC1_ALPHA = 19
 };

 enum class BLEND_OP {
   kBLEND_OP_ADD = 1,
   kBLEND_OP_SUBTRACT = 2,
   kBLEND_OP_REV_SUBTRACT = 3,
   kBLEND_OP_MIN = 4,
   kBLEND_OP_MAX = 5
 };

 enum class LOGIC_OP {
   kLOGIC_OP_CLEAR = 0,
   kLOGIC_OP_SET = (kLOGIC_OP_CLEAR + 1),
   kLOGIC_OP_COPY = (kLOGIC_OP_SET + 1),
   kLOGIC_OP_COPY_INVERTED = (kLOGIC_OP_COPY + 1),
   kLOGIC_OP_NOOP = (kLOGIC_OP_COPY_INVERTED + 1),
   kLOGIC_OP_INVERT = (kLOGIC_OP_NOOP + 1),
   kLOGIC_OP_AND = (kLOGIC_OP_INVERT + 1),
   kLOGIC_OP_NAND = (kLOGIC_OP_AND + 1),
   kLOGIC_OP_OR = (kLOGIC_OP_NAND + 1),
   kLOGIC_OP_NOR = (kLOGIC_OP_OR + 1),
   kLOGIC_OP_XOR = (kLOGIC_OP_NOR + 1),
   kLOGIC_OP_EQUIV = (kLOGIC_OP_XOR + 1),
   kLOGIC_OP_AND_REVERSE = (kLOGIC_OP_EQUIV + 1),
   kLOGIC_OP_AND_INVERTED = (kLOGIC_OP_AND_REVERSE + 1),
   kLOGIC_OP_OR_REVERSE = (kLOGIC_OP_AND_INVERTED + 1),
   kLOGIC_OP_OR_INVERTED = (kLOGIC_OP_OR_REVERSE + 1)
 };

 struct BlendStateDesc {
  struct RenderTargetBlendDesc {
    BLEND srcBlend = BLEND::kBLEND_ONE;
    BLEND destBlend = BLEND::kBLEND_ZERO;
    BLEND_OP blendOP = BLEND_OP::kBLEND_OP_ADD;
    BLEND srcsBlendAlpha = BLEND::kBLEND_ONE;
    BLEND destBlendAlpha = BLEND::kBLEND_ZERO;
    BLEND_OP blenOpAlpha = BLEND_OP::kBLEND_OP_ADD;
    LOGIC_OP logicOP = LOGIC_OP::kLOGIC_OP_CLEAR;
    ColorWriteEnableFlag renderTargetWritemask =  COLOR_WRITE_ENABLE::kCOLOR_WRITE_ENABLE_ALL;
  };
  bool alphaToCoverageEnable = false;
  bool independentBlendEnable = false;
  Array<RenderTargetBlendDesc, 8> renderTargetBlendDesc = {};
 };

 enum class PRIMITIVE_TOPOLOGY_TYPE {
   kPRIMITIVE_TOPOLOGY_TYPE_UNDEFINED = 0,
   kPRIMITIVE_TOPOLOGY_TYPE_POINT = 1,
   kPRIMITIVE_TOPOLOGY_TYPE_LINE = 2,
   kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE = 3,
   kPRIMITIVE_TOPOLOGY_TYPE_PATCH = 4
 };

 enum class DEPTH_WRITE_MASK {
   kZERO = 0,
   kALL = 1
 };

 enum class COMPARISON_FUNC {
   kNONE = 0,
   kNEVER = 1,
   kLESS = 2,
   kEQUAL = 3,
   kLESS_EQUAL = 4,
   kGREATER = 5,
   kNOT_EQUAL = 6,
   kGREATER_EQUAL = 7,
   kALWAYS = 8
 };

enum class STENCIL_OP {
  kNONE = 0,
  kKEEP = 1,
  kZERO = 2,
  kREPLACE = 3,
  kINCR_SAT = 4,
  kDECR_SAT = 5,
  kINVERT = 6,
  kINCR = 7,
  kDECR = 8
};

struct DepthStencilOpDesc {
  STENCIL_OP stencilFailOp = STENCIL_OP::kNONE;
  STENCIL_OP stencilDepthFailOp = STENCIL_OP::kNONE;
  STENCIL_OP stencilPassOp = STENCIL_OP::kNONE;
  COMPARISON_FUNC stencilFunc = COMPARISON_FUNC::kNONE;
};

struct DepthStencilStateDesc {
  bool depthEnable = false;
  DEPTH_WRITE_MASK depthWriteMask = DEPTH_WRITE_MASK::kZERO;
  COMPARISON_FUNC DepthFunc = COMPARISON_FUNC::kNONE;
  bool stencilEnable = false;
  uint8 stencilReadMask = 0;
  uint8 stencilWriteMask =  0;
  DepthStencilOpDesc FrontFace;
  DepthStencilOpDesc BackFace;
};

enum class TEXTURE_ADDRESS_MODE {
  kWRAP = 1,
  kMIRROR = 2,
  kCLAMP = 3,
  kBORDER = 4,
  kMIRROR_ONCE = 5
};

enum class FILTER {
  kNEAREST = 0,
  kLINEAR = 1,
  kCUBIC_EXT = 1000015000,
  kCUBIC_IMG = kCUBIC_EXT,
  kMAX_ENUM = 0x7FFFFFFF
};

struct SamplerDesc {
  SamplerDesc() = default;

  FILTER minFilter = FILTER::kLINEAR;
  FILTER magFilter = FILTER::kLINEAR;
  TEXTURE_ADDRESS_MODE addressU = TEXTURE_ADDRESS_MODE::kWRAP;
  TEXTURE_ADDRESS_MODE addressV = TEXTURE_ADDRESS_MODE::kWRAP;
  TEXTURE_ADDRESS_MODE addressW = TEXTURE_ADDRESS_MODE::kWRAP;
  float mipLODBias = 0.0f;
  uint32 maxAnisotropy = 1;
  bool anisotropyEnable = false;
  bool compareEnable = false;
  COMPARISON_FUNC comparisonFunc = COMPARISON_FUNC::kNONE;
  LinearColor borderColor = LinearColor::Black;
  float minLOD = -std::numeric_limits<float>::max();
  float maxLOD = std::numeric_limits<float>::max();
  FILTER mipMapMode = FILTER::kLINEAR;
  uint32 maxLODClamp = 0;
  uint32 shaderRegister = 0;
  uint32 registerSpace = 0;
};

struct MultiSampleDesc {
  SampleCountFlag count = SAMPLE_COUNT::kSAMPLE_COUNT_1;
  bool sampleShadingEnable = false;
  float minSampleShading = 0.0f;
  Vector<uint32> sampleMask;
  bool alphaToCoverageEnable = false;
  bool alphaToOneEnable = false;
};

struct RenderPassDesc {
  Array<FORMAT, 8> colorAttachments;
  FORMAT depthStencilAttachment = FORMAT::kD32_FLOAT_S8X24_UINT;
  uint32 subpassCount = 1;
};

struct PipelineStateDesc {
  SPtr<Shader> VS;
  SPtr<Shader> PS;
  SPtr<Shader> CS;
  SPtr<Shader> MS;
  BlendStateDesc blendState;
  uint32 sampleMask = 0xffffffff;
  RasterizerStateDesc rasterizerStateDesc;
  DepthStencilStateDesc depthStencilStateDesc;
  Vector<VertexBufferBindingDesc> vertexBufferBindingsDesc;
  BindingGroup bindingGroups;
  PRIMITIVE_TOPOLOGY_TYPE topology = PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  uint32 numRenderTextures = 1;
  RenderPassDesc renderPassDesc;
  MultiSampleDesc sampleDesc;
  Box2D viewport;
};

struct VertexBufferDesc {
  uint32 strideInBytes;
  uint32 size;
};

struct IndexBufferDesc {
  FORMAT format;
  uint32 size;
};

enum class COMMAND_BUFFER_TYPES {
   kDIRECT = 0,
   kBUNDLE = 1,
   kCOMPUTE = 2,
   kCOPY = 3,
   kVIDEO_DECODE = 4,
   kVIDEO_PROCESS = 5,
   kVIDEO_ENCODE
 };

enum class BarrierType {
  kTRANSITION = 0,
  kALIASING,
  kUAV
};

//TODO: not sure if this should be a flag. For know I just copy paste from DX12 xd 
enum class ResourceStates {
//   kCOMMON = 0,
//   kVERTEX_AND_CONSTANT_BUFFER = 0x1,
//   kINDEX_BUFFER = 0x2,
//   kUNORDERED_ACCESS = 0x8,
//   kDEPTH_WRITE = 0x10,
//   kDEPTH_READ = 0x20,
//   kNON_PIXEL_SHADER_RESOURCE = 0x40,
//   kPIXEL_SHADER_RESOURCE = 0x80,
//   kSTREAM_OUT = 0x100,
//   kINDIRECT_ARGUMENT = 0x200,
//   kCOPY_DEST = 0x400,
//   kCOPY_SOURCE = 0x800,
//   kRESOLVE_DEST = 0x1000,
//   kRESOLVE_SOURCE = 0x2000,
//   kRAYTRACING_ACCELERATION_STRUCTURE = 0x400000,
//   kSHADING_RATE_SOURCE = 0x1000000,
//   kGENERIC_READ = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
  kPRESENT = 0,
  kPREDICATION = 0x200,
  kVIDEO_DECODE_READ = 0x10000,
  kVIDEO_DECODE_WRITE = 0x20000,
  kVIDEO_PROCESS_READ = 0x40000,
  kVIDEO_PROCESS_WRITE = 0x80000,
  kVIDEO_ENCODE_READ = 0x200000,
  kVIDEO_ENCODE_WRITE = 0x800000,
  kRENDER_TARGET,
};

struct ResourceBarrierTransition {
  SPtr<GPUResource>  resource;
  uint32 subresource;
  ResourceStates stateBefore;
  ResourceStates stateAfter;
};

struct GPUBarrier {
  BarrierFlag flag = BARRIER_FLAG::kNONE;
  //TODO: we only manage resource transition for now, we gotta find a way to handle aliasing and that shit.
  //union{ Aliasing resource ans shit };
  ResourceBarrierTransition transition;
 private:
  BarrierType type = BarrierType::kTRANSITION;
};

enum class SWAPCHAIN_EFFECT {
  DISCARD = 0,
  SEQUENTIAL = 1,
  FLIP_SEQUENTIAL = 3,
  FLIP_DISCARD = 4
};

struct SwapChainDesc {
  uint32 width = 4;
  uint32 height = 4;
  FORMAT format = FORMAT::kR8G8B8A8_UNORM;
  bool stereo = false;
  uint32 frameCount = 3;
  SWAPCHAIN_EFFECT effect = SWAPCHAIN_EFFECT::FLIP_DISCARD;
};

}
}
