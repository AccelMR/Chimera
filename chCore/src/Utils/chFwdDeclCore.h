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

class GPUCommandBuffer;
class GPUPipelineState;

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
