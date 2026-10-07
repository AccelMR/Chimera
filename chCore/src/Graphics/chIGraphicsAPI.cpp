/************************************************************************/
/**
 * @file chIGraphicsAPI.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Shared code of every graphics API.
 */
/************************************************************************/
#include "chIGraphicsAPI.h"

#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chStringUtils.h"

namespace chEngineSDK {
namespace {
struct ShaderStageNames
{
  const ANSICHAR* suffix;
  const ANSICHAR* entryPoint;
};

// Must match the names cmake/chShaders.cmake gives every entry point it compiles.
NODISCARD ShaderStageNames
getStageNames(ShaderStage stage)
{
  switch (stage) {
  case ShaderStage::Vertex:
    return {"vs", "VSMain"};
  case ShaderStage::Fragment:
    return {"ps", "PSMain"};
  case ShaderStage::Compute:
    return {"cs", "CSMain"};
  default:
    CH_ASSERT(false && "The build compiles only vertex, pixel and compute shaders.");
    return {"", ""};
  }
}
} // namespace

/*
 */
SPtr<IShader>
IGraphicsAPI::loadShader(ShaderStage stage, StringView name)
{
  const ShaderBinaryFormat format = getShaderBinaryFormat();
  const ShaderStageNames stageNames = getStageNames(stage);
  const String fileName =
      StringUtils::format("{0}.{1}.{2}", name, stageNames.suffix, format.extension);
  const Path path(EnginePaths::getShaderBinaryDirectory(), Path(String(format.folder)),
                  Path(fileName));

  return createShader({.stage = stage,
                       .entryPoint = stageNames.entryPoint,
                       .sourceCode = FileSystem::fastRead(path),
                       .filePath = path.toString(),
                       .defines = {}});
}

} // namespace chEngineSDK
