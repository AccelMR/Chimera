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
/**
 * Limits shared by every graphics API, so the engine and the shaders agree on them.
 */
class GraphicsLimits
{
 public:
  static constexpr uint32 MAX_COLOR_ATTACHMENTS = 8;
  // The push constant size every supported graphics API guarantees.
  static constexpr uint32 PUSH_CONSTANTS_SIZE = 128;
  static constexpr uint32 MAX_BINDLESS_RESOURCES = 65536;
  // The smallest sampler heap among the supported graphics APIs.
  static constexpr uint32 MAX_BINDLESS_SAMPLERS = 2048;
  static constexpr uint32 INVALID_BINDLESS_INDEX = ~0u;
  // The CPU records one frame while the GPU runs the one before it.
  static constexpr uint32 MAX_FRAMES_IN_FLIGHT = 2;
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

/**
 * Texture formats. Texture assets store the value, so every value is fixed: new formats go
 * at the end and none is ever reordered.
 */
enum class Format : uint32
{
  Unknown = 0,
  R8G8B8A8_UNORM = 1,
  B8G8R8A8_UNORM = 2,
  B8G8R8A8_SRGB = 3,
  R16G16B16A16_SFLOAT = 4,
  D32_SFLOAT = 5,
  D24_UNORM_S8_UINT = 6,
  R8_UNORM = 7,
  R8G8_UNORM = 8,
  R8G8B8A8_SRGB = 9,
  A2B10G10R10_UNORM = 10,
  B10G11R11_UFLOAT = 11,
  R16_SFLOAT = 12,
  R16G16_SFLOAT = 13,
  R32_SFLOAT = 14,
  R32G32B32A32_SFLOAT = 15,
  R32_UINT = 16,
  D32_SFLOAT_S8_UINT = 17,

  COUNT
};

/**
 * What the engine needs to know about a format to size, copy and bind it. Block sizes are
 * there for compressed formats; every format today is one pixel per block.
 */
struct FormatInfo
{
  uint8 bytesPerBlock = 0;
  uint8 blockWidth = 1;
  uint8 blockHeight = 1;
  bool isDepth = false;
  bool hasStencil = false;
  bool isSrgb = false;
};

/**
 * Looks up FormatInfo by format. The table is constexpr, so a lookup is one array read.
 */
class FormatUtils
{
 public:
  NODISCARD static constexpr const FormatInfo&
  getInfo(Format format)
  {
    return FORMAT_INFOS[static_cast<uint32>(format)];
  }

  NODISCARD static constexpr bool
  isDepth(Format format)
  {
    return getInfo(format).isDepth;
  }

  /**
   * Bytes of one mip level of one layer, rounded up to whole blocks.
   */
  NODISCARD static constexpr SIZE_T
  getMipSize(Format format, uint32 width, uint32 height, uint32 depth = 1)
  {
    const FormatInfo& info = getInfo(format);
    const SIZE_T blocksWide = (width + info.blockWidth - 1) / info.blockWidth;
    const SIZE_T blocksHigh = (height + info.blockHeight - 1) / info.blockHeight;
    return blocksWide * blocksHigh * depth * info.bytesPerBlock;
  }

 private:
  // In the order of the Format values.
  static constexpr FormatInfo FORMAT_INFOS[] = {
      {},                                                  // Unknown
      {.bytesPerBlock = 4},                                // R8G8B8A8_UNORM
      {.bytesPerBlock = 4},                                // B8G8R8A8_UNORM
      {.bytesPerBlock = 4, .isSrgb = true},                // B8G8R8A8_SRGB
      {.bytesPerBlock = 8},                                // R16G16B16A16_SFLOAT
      {.bytesPerBlock = 4, .isDepth = true},               // D32_SFLOAT
      {.bytesPerBlock = 4, .isDepth = true, .hasStencil = true}, // D24_UNORM_S8_UINT
      {.bytesPerBlock = 1},                                // R8_UNORM
      {.bytesPerBlock = 2},                                // R8G8_UNORM
      {.bytesPerBlock = 4, .isSrgb = true},                // R8G8B8A8_SRGB
      {.bytesPerBlock = 4},                                // A2B10G10R10_UNORM
      {.bytesPerBlock = 4},                                // B10G11R11_UFLOAT
      {.bytesPerBlock = 2},                                // R16_SFLOAT
      {.bytesPerBlock = 4},                                // R16G16_SFLOAT
      {.bytesPerBlock = 4},                                // R32_SFLOAT
      {.bytesPerBlock = 16},                               // R32G32B32A32_SFLOAT
      {.bytesPerBlock = 4},                                // R32_UINT
      {.bytesPerBlock = 8, .isDepth = true, .hasStencil = true}, // D32_SFLOAT_S8_UINT
  };
  static_assert(std::size(FORMAT_INFOS) == static_cast<SIZE_T>(Format::COUNT),
                "Every Format needs its FormatInfo.");
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

  const void* initialData = nullptr;
  SIZE_T initialDataSize = 0;
};

struct TextureViewCreateInfo {
  Format format = Format::Unknown; // Unknown takes the format of the texture.
  TextureViewType viewType = TextureViewType::View2D;
  uint32 baseMipLevel = 0;
  uint32 mipLevelCount = ~0u;
  uint32 baseArrayLayer = 0;
  uint32 arrayLayerCount = ~0u;
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
  // The engine is left-handed with NDC Y up, so front faces are clockwise.
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
  RasterState raster{};
  DepthState depth{};

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
 * Targets of one ICommandList::beginRendering. Kept on the stack: no allocation per pass.
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

} // namespace chEngineSDK
