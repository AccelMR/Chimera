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

 enum class ACCESS_FLAG {
  kNONE = 0,
  kCOLOR_ATTACHMENT_WRITE = 0x1,
  kCOLOR_ATTACHMENT_READ = 0x2,
  kDEPTH_STENCIL_WRITE = 0x4,
  kDEPTH_STENCIL_READ = 0x8,
  kSHADER_READ = 0x10
};
CH_FLAGS_OPERATORS_EXT(ACCESS_FLAG, uint32);
using AccessFlag = Flags<ACCESS_FLAG, uint32>;

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

struct DescriptorBinding{
  enum class TYPE {
    kUNIFORM_BUFFER,
    kSTORAGE_BUFFER,
    kSAMPLED_TEXTURE,
    kSTORAGE_TEXTURE,
    kSAMPLER
  };

  TYPE type = TYPE::kUNIFORM_BUFFER;
  ShaderStageFlag stages = SHADER_STAGE::ALL;
  uint32 slot = 0;

  Variant<SPtr<GPUBuffer>, SPtr<Texture>, SPtr<Sampler>> resource;

  DescriptorBinding(TYPE _type, 
                   ShaderStageFlag _stages,
                   uint32 _slot, 
                   SPtr<GPUBuffer> _buffer) :
    type(_type),
    stages(_stages),
    slot(_slot),
    resource(_buffer) {}

  DescriptorBinding(TYPE _type, 
                   ShaderStageFlag _stages,
                   uint32 _slot, 
                   SPtr<Texture> _texture) :
    type(_type),
    stages(_stages),
    slot(_slot),
    resource(_texture) {}

  DescriptorBinding(TYPE _type, 
                   ShaderStageFlag _stages,
                   uint32 _slot, 
                   SPtr<Sampler> _sampler) :
    type(_type),
    stages(_stages),
    slot(_slot),
    resource(_sampler) {}

  template<typename T>
  SPtr<T> getResource() {
    CH_ASSERT(std::holds_alternative<SPtr<T>>(resource));
    return std::get<SPtr<T>>(resource);
  }
};

/** 
 *   Describes the bindings for a shader or group of shader.
**/
struct BindingGroup {
  uint32 descriptorSetIndex = 0;
  Vector<DescriptorBinding> bindings;

  BindingGroup(uint32 _descriptorSetIndex, 
               std::initializer_list<DescriptorBinding> _bindings) :
    descriptorSetIndex(_descriptorSetIndex),
    bindings(_bindings) {}
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
  Vector<uint32> sampleMask = { 0xffffffff };
  bool alphaToCoverageEnable = false;
  bool alphaToOneEnable = false;
};

struct ViewportDesc {
  Box2D rect;
  float minDepth = 0.0f;
  float maxDepth = 1.0f;
};

struct AttachmentDesc {
    enum class LOAD_OP {
    kLOAD,
    kCLEAR,
    kDONT_CARE
  };

  enum class STORE_OP {
    kSTORE,
    kDONT_CARE
  };

  FORMAT format = FORMAT::kUNKNOWN;
  SampleCountFlag sampleCount = SAMPLE_COUNT::kSAMPLE_COUNT_1;
  LOAD_OP loadOp = LOAD_OP::kCLEAR;
  STORE_OP storeOp = STORE_OP::kSTORE;
  bool isResolveAttachment = false;
};

struct SubpassDesc {
  Vector<uint32> inputAttachments;
  Vector<uint32> colorAttachments;
  Optional<uint32> depthStencilAttachment;
  Vector<uint32> resolveAttachments;
};

struct SubpassDependency {
  uint32 srcSubpass = 0;
  uint32 dstSubpass = 1;

  AccessFlag srcAccessMask;
  AccessFlag dstAccessMask;
};

struct RenderPassDesc {
  Vector<AttachmentDesc> attachments;
  Vector<SubpassDesc> subpasses;
  Vector<SubpassDependency> dependencies;
};

struct PipelineStateDesc {
  SPtr<Shader> VS;
  SPtr<Shader> PS;
  SPtr<Shader> CS;
  SPtr<Shader> MS;
  BlendStateDesc blendState;
  RasterizerStateDesc rasterizerStateDesc;
  DepthStencilStateDesc depthStencilStateDesc;
  Vector<VertexBufferBindingDesc> vertexBufferBindingsDesc;
  Vector<BindingGroup> bindingGroups;
  PRIMITIVE_TOPOLOGY_TYPE topology = PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  RenderPassDesc renderPassDesc;
  MultiSampleDesc sampleDesc;
  Vector<Box2D> viewports;
  Vector<Box2D> scissorRects;
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

enum class ResourceStates {
  kPRESENT = 0,
  kRENDER_TARGET,
  kSHADER_RESOURCE,
  kUNORDERED_ACCESS,
  kDEPTH_STENCIL,
  kCOPY_DEST,
  kCOPY_SOURCE,

  kCOUNT
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
  bool vsyncEnabled;
};

}
}
