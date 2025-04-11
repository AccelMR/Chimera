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
enum class KEYBOARD_STATE : uint32;
enum class KEY_MODIFIER : uint16;

// Forward of any graphics stuff
class IGraphicsAPI;
class ISwapChain;
class ICommandQueue;
class ICommandPool;
class ICommandBuffer;
class IFence;
class IBuffer;
class ITexture;
class ITextureView;
class IShader;
class IPipeline;
class IRenderPass;
class IFrameBuffer;
class ISemaphore;

} // namespace chEngineSDK
