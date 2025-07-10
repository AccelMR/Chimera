/************************************************************************/
/**
 * @file chSupportedImport.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 *  This file contains the supported import types for the engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chIAsset.h"

namespace chEngineSDK {
static const UnorderedMap<AssetType, Vector<String>> SUPPORTED_IMPORTS = {
  {AssetType::Model, {"fbx", "obj", "gltf", "glb", "dae", "3ds", "ase", "ifc", "xgl", "zgl", "ply", "lwo", "lws", "stl", "x", "ac", "ms3d", "cob", "scn", "blend"}},
  {AssetType::Texture, {"png", "jpg", "jpeg", "tga"}},
  {AssetType::Shader, {"hlsl", "glsl"}},
  {AssetType::Material, {""}},
  {AssetType::Prefab, {""}},
};
} // namespace chEngineSDK