/************************************************************************/
/**
 * @file chPrerequisitesGraphics.h
 * @author AccelMR
 * @date 2022/11/13
 *   Prerequisites that mus be included for every graphics related class.
 */
 /************************************************************************/


#include <chFlags.h>

namespace chEngineSDK {
namespace chGPUDesc {
  enum class PRIMITIVE_TOPOLOGY_TYPE;
  enum class COMMAND_BUFFER_TYPES;
  struct GPUBarrier;
  struct TextureDesc;
  struct PipelineStateDesc;
  struct VertexBufferDesc;
  struct IndexBufferDesc;
  struct BufferBindingDesc;
  struct ShaderDesc;
  struct SwapChainDesc;
  struct BindingGroup;
  struct GPUPipelineStateDesc;
}
class GPUResource;
class GPUBuffer;
class Texture;
class IndexBuffer;
class VertexBuffer;
class GPUPipelineState;
class Sampler;
class SwapChain;
class Shader;
class Fence;
class RenderPass;
class Framebuffer;
}