/************************************************************************/
/**
 * @file chGraphicsTypes.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 *  Graphics types and enums used in the graphics API.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chFlags.h"
#include "chLinearColor.h"
#include "chVertexLayout.h"

namespace chEngineSDK {
constexpr uint32 SUBPASS_EXTERNAL = ~0u;

enum class QueueType : uint32 {
  Graphics = 0,
  Compute,
  Transfer,
  Present,

  Count
};

enum class CompareOp {
  Never,
  Less,
  Equal,
  LessOrEqual,
  Greater,
  NotEqual,
  GreaterOrEqual,
  AlwaysOp
};

enum class BlendFactor {
  Zero,
  One,
  SrcColor,
  OneMinusSrcColor,
  DstColor,
  OneMinusDstColor,
  SrcAlpha,
  OneMinusSrcAlpha,
  DstAlpha,
  OneMinusDstAlpha,
  // More factors ...
};

enum class BlendOp {
  Add,
  Subtract,
  ReverseSubtract,
  Min,
  Max
};

enum class IndexType : uint32 {
  UInt16 = 0,
  UInt32,

  COUNT
};

enum class Format : uint32 {
  Unknown = 0,
  R8G8B8A8_UNORM,
  B8G8R8A8_SRGB,
  R16G16B16A16_SFLOAT,
  D32_SFLOAT,
  D24_UNORM_S8_UINT,

  COUNT
};

enum class LoadOp : uint32 {
  Load = 0,
  Clear,
  DontCare,

  COUNT
};

enum class StoreOp : uint32 {
  Store = 0,
  DontCare,

  COUNT
};

enum class TextureType {
  Texture1D,
  Texture2D,
  Texture3D,
  TextureCube
};

enum class TextureUsage : uint16 {
  NoneUsage         = 0,
  TransferSrc       = 1 << 0,
  TransferDst       = 1 << 1,
  Sampled           = 1 << 2,
  Storage           = 1 << 3,
  ColorAttachment   = 1 << 4,
  DepthStencil      = 1 << 5,
  Transient         = 1 << 6,
  InputAttachment   = 1 << 7 
};
CH_FLAGS_OPERATORS_EXT(TextureUsage, uint16);
using TextureUsageFlags = Flags<TextureUsage, uint16>;

enum class TextureViewType {
  View1D,
  View2D,
  View3D,
  ViewCube,
  View1DArray,
  View2DArray,
  ViewCubeArray
};

enum class SampleCount {
  Count1  = 1,
  Count2  = 2,
  Count4  = 4,
  Count8  = 8,
  Count16 = 16,
  Count32 = 32,
  Count64 = 64
};

enum class PrimitiveTopology {
  PointList,
  LineList,
  LineStrip,
  TriangleList,
  TriangleStrip
};

enum class ShaderStage {
  Vertex,
  Fragment,
  Compute,
  Geometry,
  TessControl,
  TessEvaluation
};

enum class BufferUsage : uint16 {
  VertexBuffer = 0x01,
  IndexBuffer = 0x02,
  UniformBuffer = 0x04,
  StorageBuffer = 0x08,
  TransferSrc = 0x10,
  TransferDst = 0x20
};
CH_FLAGS_OPERATORS_EXT(BufferUsage, uint16);
using BufferUsageFlags = Flags<BufferUsage, uint16>;

enum class MemoryUsage {
  GpuOnly,
  CpuOnly,
  CpuToGpu,
  GpuToCpu
};

enum class TextureLayout {
  Undefined,                
  General,                  
  ColorAttachment,          
  DepthStencilAttachment,   
  DepthStencilReadOnly,     
  ShaderReadOnly,           
  TransferSrc,              
  TransferDst,              
  PresentSrc,               
  // Otros layouts específicos...
};

enum class PipelineBindPoint {
  Graphics,
  Compute
};

enum class PipelineStage : uint32 {
  None                    = 0,
  TopOfPipe               = 1 << 0,
  DrawIndirect            = 1 << 1,
  VertexInput             = 1 << 2,
  VertexShader            = 1 << 3,
  FragmentShader          = 1 << 4,
  ColorAttachmentOutput   = 1 << 5,
  ComputeShader           = 1 << 6,
  Transfer                = 1 << 7,
  BottomOfPipe            = 1 << 8,
  AllGraphics             = 1 << 9,
  AllCommands             = 1 << 10
};
CH_FLAGS_OPERATORS_EXT(PipelineStage, uint32);
using PipelineStageFlags = Flags<PipelineStage, uint32>;

enum class Access : uint32 {
  NoAccess                      = 0,
  ShaderRead                = 1 << 0,
  ShaderWrite               = 1 << 1,
  ColorAttachmentRead       = 1 << 2,
  ColorAttachmentWrite      = 1 << 3,
  DepthStencilAttachmentRead = 1 << 4,
  DepthStencilAttachmentWrite = 1 << 5,
  TransferRead              = 1 << 6,
  TransferWrite             = 1 << 7,
  HostRead                  = 1 << 8,
  HostWrite                 = 1 << 9,
  MemoryRead                = 1 << 10,
  MemoryWrite               = 1 << 11
};
CH_FLAGS_OPERATORS_EXT(Access, uint32);
using AccessFlags = Flags<Access, uint32>;

struct AttachmentReference {
  uint32 attachment = ~0u;
  TextureLayout layout = TextureLayout::Undefined;
};

struct SubpassDescription {
  PipelineBindPoint pipelineBindPoint = PipelineBindPoint::Graphics;
  Vector<AttachmentReference> inputAttachments;
  Vector<AttachmentReference> colorAttachments;
  Vector<AttachmentReference> resolveAttachments;
  Optional<AttachmentReference> depthStencilAttachment;
  Vector<uint32> preserveAttachments;
};

struct SubpassDependency {
  uint32 srcSubpass = ~0u;
  uint32 dstSubpass = ~0u;
  PipelineStage srcStageMask = PipelineStage::ColorAttachmentOutput;
  PipelineStage dstStageMask = PipelineStage::ColorAttachmentOutput;
  AccessFlags srcAccessMask = Access::ColorAttachmentWrite;
  AccessFlags dstAccessMask = Access::ColorAttachmentWrite;
  bool byRegion = false;
};

struct TextureCreateInfo {
  TextureType type = TextureType::Texture2D;
  Format format = Format::R8G8B8A8_UNORM;
  uint32 width = 1;
  uint32 height = 1;
  uint32 depth = 1;
  uint32 mipLevels = 1;
  uint32 arrayLayers = 1;
  SampleCount samples = SampleCount::Count1;
  
  TextureUsageFlags usage = TextureUsage::Sampled | TextureUsage::TransferDst;
  
  bool cubeCompatible = false;
  bool generateMipmaps = false;
  
  bool concurrentSharing = false;
  Vector<uint32> queueFamilyIndices;
  
  const void* initialData = nullptr;
  SIZE_T initialDataSize = 0;
};

struct TextureViewCreateInfo {
  Format format = Format::Unknown;  // Usar el formato de la imagen si es Unknown
  TextureViewType viewType = TextureViewType::View2D;
  uint32 baseMipLevel = 0;
  uint32 mipLevelCount = ~0u;
  uint32 baseArrayLayer = 0;
  uint32 arrayLayerCount = ~0u;
};

struct TextireCreateInfo {
  TextureType type = TextureType::Texture2D;
  Format format = Format::Unknown;
  uint32 width = 0;
  uint32 height = 0;
  uint32 depth = 1;
  uint32 mipLevels = 1;
};

struct VertexPosColor {
  float position[3];
  float color[4];
  
  static VertexLayout getLayout() {
    return VertexLayout::createPostionColorLayout();
  }
};

struct ShaderCreateInfo {
  ShaderStage stage;
  String entryPoint;
  Vector<uint8> sourceCode; // Source code in binary format
  String filePath;   // File path for the shader source code
  Vector<String> defines; // Preprocessor defines for the shader
};

struct PipelineCreateInfo {
  Map<ShaderStage, SPtr<IShader>> shaders;
  VertexLayout vertexLayout;
  PrimitiveTopology topology = PrimitiveTopology::TriangleList;

  struct {
    bool enable = false;
    bool writeEnable = true;
    CompareOp compareOp = CompareOp::Less;
  } depthStencil;

  struct {
    bool enable = false;
    BlendFactor srcColorFactor = BlendFactor::SrcAlpha;
    BlendFactor dstColorFactor = BlendFactor::OneMinusSrcAlpha;
    // TODO: Add more blend factors
  } blend;

  SPtr<IRenderPass> renderPass;
  uint32 subpass = 0;

  //TODO: Add more fields for dynamic states, push constants, etc.
};


struct BufferCreateInfo {
  uint32 size = 0;
  BufferUsageFlags usage = BufferUsage::UniformBuffer;
  MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
  const void* initialData = nullptr;
  SIZE_T initialDataSize = 0;
};

struct AttachmentDescription {
  Format format;
  LoadOp loadOp;
  StoreOp storeOp;
  LoadOp stencilLoadOp;
  StoreOp stencilStoreOp;
  TextureLayout initialLayout;
  TextureLayout finalLayout;
};

struct RenderPassCreateInfo {
  Vector<AttachmentDescription> attachments;
  Vector<SubpassDescription> subpasses;
  Vector<SubpassDependency> dependencies;
};

struct FrameBufferCreateInfo {
  SPtr<IRenderPass> renderPass;
  Vector<SPtr<ITextureView>> attachments;
  uint32 width;
  uint32 height;
  uint32 layers = 1;
};

struct RenderPassBeginInfo {
  SPtr<IRenderPass> renderPass;
  SPtr<IFrameBuffer> framebuffer;
  Vector<LinearColor> clearValues;
};

struct SubmitInfo {
  Vector<SPtr<ICommandBuffer>> commandBuffers;
  Vector<SPtr<ISemaphore>> waitSemaphores;
  Vector<PipelineStageFlags> waitStages;
  Vector<SPtr<ISemaphore>> signalSemaphores;
};

} // namespace chEngineSDK