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

class MeshCodec;
class Mesh;
class Model;

// Forward of any graphics stuff
class IBuffer;
class ICommandList;
class IGraphicsAPI;
class IPipeline;
class ISampler;
class IShader;
class ISwapChain;
class ITexture;
class ITextureView;

class IAsset;
class AssetRegister;
struct AssetMetadata;
class IAssetManager;

class Scene;
class SceneAsset;
class SceneManager;
class GameObject;
class Transform;

} // namespace chEngineSDK
