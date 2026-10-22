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
class Component;
class GameObject;
class ModelComponent;
struct LightItem;
struct RenderItem;
class RenderFrame;
class Transform;

} // namespace chEngineSDK
