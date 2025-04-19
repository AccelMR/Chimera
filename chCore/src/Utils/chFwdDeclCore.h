/************************************************************************/
/**
 * @file chFwdDeclCore.h
 * @author AccelMR
 * @date 2022/09/11
 *   Forward declarations from Core
 */
 /************************************************************************/
#pragma once

namespace chEngineSDK{
class DisplayManager;
class DisplaySurface;
class DisplayEventHandle;
class DisplayEvent;

struct MouseMoveData;
struct KeyBoardData;
enum class KeyBoardState : uint32;
enum class KeyBoardModifier : uint16;

class Camera;

// Forward of any graphics stuff
class IBuffer;
class ICommandBuffer;
class ICommandPool;
class ICommandQueue;
class IDescriptorPool;
class IDescriptorSet;
class IDescriptorSetLayout;
class IFence;
class IFrameBuffer;
class IGraphicsAPI;
class IPipeline;
class IPipelineLayout;
class IRenderPass;
class ISampler;
class ISemaphore;
class IShader;
class ISwapChain;
class ITexture;
class ITextureView;



} // namespace chEngineSDK
