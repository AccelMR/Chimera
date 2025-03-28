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
class ScreenModule;
class Screen;
class ScreenEventHandle;
class ScreenEvent;

class GPUCommandBuffer;
class GPUPipelineState;

union ScreenEventData;
struct MouseMoveData;
struct KeyBoardData;

namespace chGPUDesc {
struct PipelineStateDesc;
struct ShaderDesc;
struct DepthStencilStateDesc;
struct DepthStencilOpDesc;
struct VertexBufferBindingDesc;
struct RenderPassDesc;
}
}
