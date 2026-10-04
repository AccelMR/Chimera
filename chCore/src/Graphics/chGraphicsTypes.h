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

/**
 * Limits shared by every graphics API, so the engine and the shaders agree on them.
 */
class GraphicsLimits
{
 public:
  static constexpr uint32 MAX_COLOR_ATTACHMENTS = 8;
  // The size Vulkan guarantees on every device; DX12 root constants hold it too.
  static constexpr uint32 PUSH_CONSTANTS_SIZE = 128;
  static constexpr uint32 MAX_BINDLESS_RESOURCES = 65536;
  // The size of the DX12 sampler heap.
  static constexpr uint32 MAX_BINDLESS_SAMPLERS = 2048;
  static constexpr uint32 INVALID_BINDLESS_INDEX = ~0u;
};

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
  B8G8R8A8_UNORM,
  B8G8R8A8_SRGB,
  R16G16B16A16_SFLOAT,
  D32_SFLOAT,
  D24_UNORM_S8_UINT,

  COUNT
};

NODISCARD FORCEINLINE constexpr bool
isDepthFormat(Format format)
{
  return format == Format::D32_SFLOAT || format == Format::D24_UNORM_S8_UINT;
}

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

enum class ShaderStage : uint32 {
  Vertex = 0x01,
  Fragment = 0x02,
  Compute = 0x04,
  Geometry = 0x08,
  TessControl = 0x10,
  TessEvaluation = 0x20,
};
CH_FLAGS_OPERATORS_EXT(ShaderStage, uint32);
using ShaderStageFlags = Flags<ShaderStage, uint32>;

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

/**
 * What a texture is used for at a point of the frame. A barrier moves it from one state to
 * the next; each graphics API turns the pair into its own stages, accesses and layouts.
 */
enum class ResourceState : uint32
{
  Undefined,      // Contents are not needed (first use, or about to be overwritten).
  RenderTarget,
  DepthWrite,
  DepthRead,
  ShaderRead,
  UnorderedAccess,
  CopySource,
  CopyDestination,
  Present
};

enum class CullMode : uint32
{
  None,
  Front,
  Back
};

enum class FrontFace : uint32
{
  Clockwise,
  CounterClockwise
};

enum class PolygonMode : uint32
{
  Fill,
  Line
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

enum class SamplerAddressMode {
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder,
  MirrorClampToEdge
};

enum class SamplerFilter {
  Nearest,
  Linear
};

enum class SamplerMipmapMode {
  Nearest,
  Linear
};

struct SamplerCreateInfo {
  SamplerFilter magFilter = SamplerFilter::Linear;
  SamplerFilter minFilter = SamplerFilter::Linear;
  SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;
  SamplerAddressMode addressModeU = SamplerAddressMode::Repeat;
  SamplerAddressMode addressModeV = SamplerAddressMode::Repeat;
  SamplerAddressMode addressModeW = SamplerAddressMode::Repeat;
  float mipLodBias = 0.0f;
  bool anisotropyEnable = false;
  float maxAnisotropy = 1.0f;
  bool compareEnable = false;
  CompareOp compareOp = CompareOp::AlwaysOp;
  float minLod = 0.0f;
  float maxLod = 1000.0f;
  LinearColor borderColor = LinearColor::Black;
  bool unnormalizedCoordinates = false;
};

struct AttachmentReference {
  uint32 attachment = ~0u;
  TextureLayout layout = TextureLayout::Undefined;
};

struct SubpassDescription {
  PipelineBindPoint pipelineBindPoint = PipelineBindPoint::Graphics;
  Vector<AttachmentReference> inputAttachments = {/**/};
  Vector<AttachmentReference> colorAttachments = {/**/};
  Vector<AttachmentReference> resolveAttachments = {/**/};
  Optional<AttachmentReference> depthStencilAttachment = NullOpt;
  Vector<uint32> preserveAttachments = {/**/};
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
  Vector<uint32> queueFamilyIndices = {/**/};

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
  bool bIsDepthStencil = false;
};

struct ShaderCreateInfo {
  ShaderStage stage;
  String entryPoint;
  Vector<uint8> sourceCode; // Source code in binary format
  String filePath;   // File path for the shader source code
  Vector<String> defines; // Preprocessor defines for the shader
};

struct BlendAttachmentState {
  bool enable = false;
  BlendFactor srcColorFactor = BlendFactor::SrcAlpha;
  BlendFactor dstColorFactor = BlendFactor::OneMinusSrcAlpha;
  BlendOp colorOp = BlendOp::Add;
  BlendFactor srcAlphaFactor = BlendFactor::One;
  BlendFactor dstAlphaFactor = BlendFactor::Zero;
  BlendOp alphaOp = BlendOp::Add;
};

struct RasterState {
  CullMode cullMode = CullMode::Back;
  // The engine is left-handed and projects like Direct3D, so front faces are clockwise.
  FrontFace frontFace = FrontFace::Clockwise;
  PolygonMode polygonMode = PolygonMode::Fill;
  float depthBiasConstant = 0.0f;
  float depthBiasSlope = 0.0f;
};

struct DepthState {
  bool testEnable = true;
  bool writeEnable = true;
  CompareOp compareOp = CompareOp::Less;
};

/**
 * Everything a graphics pipeline is built from. Every pipeline shares one layout (the
 * bindless heap plus push constants), so there is nothing about resources here, and the
 * attachment formats replace a render pass.
 */
struct CH_CORE_EXPORT GraphicsPipelineDesc {
  SPtr<IShader> vertexShader;
  SPtr<IShader> fragmentShader;
  VertexLayout vertexLayout;
  PrimitiveTopology topology = PrimitiveTopology::TriangleList;
  RasterState raster;
  DepthState depth;

  Array<Format, GraphicsLimits::MAX_COLOR_ATTACHMENTS> colorFormats{};
  Array<BlendAttachmentState, GraphicsLimits::MAX_COLOR_ATTACHMENTS> blendStates{};
  uint32 colorAttachmentCount = 0;
  Format depthFormat = Format::Unknown;
  SampleCount samples = SampleCount::Count1;

  /**
   * Key for the pipeline cache. Shaders count by identity, so two descriptions with the
   * same shader objects and state give the same key.
   */
  NODISCARD uint64
  getHash() const;
};

struct ColorAttachment {
  const ITextureView* view = nullptr;
  LoadOp loadOp = LoadOp::Clear;
  StoreOp storeOp = StoreOp::Store;
  LinearColor clearColor = LinearColor::Black;
};

struct DepthAttachment {
  const ITextureView* view = nullptr;
  LoadOp loadOp = LoadOp::Clear;
  StoreOp storeOp = StoreOp::DontCare;
  float clearDepth = 1.0f;
};

/**
 * Targets of one ICommandBuffer::beginRendering. Kept on the stack: no allocation per pass.
 */
struct RenderingDesc {
  Array<ColorAttachment, GraphicsLimits::MAX_COLOR_ATTACHMENTS> colorAttachments{};
  uint32 colorAttachmentCount = 0;
  DepthAttachment depthAttachment{};
  uint32 width = 0;
  uint32 height = 0;
};

/**
 * Moves every mip and layer of a texture from one state to another.
 */
struct TextureBarrier {
  const ITexture* texture = nullptr;
  ResourceState before = ResourceState::Undefined;
  ResourceState after = ResourceState::Undefined;
};


struct BufferCreateInfo {
  uint32 size = 0;
  BufferUsageFlags usage = BufferUsage::UniformBuffer;
  MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
  const void* initialData = nullptr;
  SIZE_T initialDataSize = 0;
};

struct AttachmentDescription {
  Format format = Format::Unknown;
  LoadOp loadOp = LoadOp::DontCare;
  StoreOp storeOp = StoreOp::DontCare;
  LoadOp stencilLoadOp = LoadOp::DontCare;
  StoreOp stencilStoreOp = StoreOp::DontCare;
  TextureLayout initialLayout = TextureLayout::Undefined;
  TextureLayout finalLayout = TextureLayout::Undefined;
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
  Optional<std::pair<float, uint32>> depthStencilClearValue = NullOpt;
};

struct SubmitInfo {
  Vector<SPtr<ICommandBuffer>> commandBuffers;
  Vector<SPtr<ISemaphore>> waitSemaphores;
  Vector<PipelineStageFlags> waitStages;
  Vector<SPtr<ISemaphore>> signalSemaphores;
};

} // namespace chEngineSDK
