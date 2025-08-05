/************************************************************************/
/**
 * @file chIMaterial.h
 * @author AccelMR
 * @date 2025/08/02
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class UUID;


// Material parameter system
struct MaterialParameter {
  enum class Type { Float, Vector2, Vector3, Vector4, Matrix4, Texture, Color, LinearColor };

  Type type;
  // Variant<float,
  //         Vector2,
  //         Vector3,
  //         Vector4,
  //         Matrix4,
  //         SPtr<ITextureView>,
  //         Color,
  //         LinearColor>
  //     value;
};


/**
 * @brief Interface for material properties
 */
class CH_CORE_EXPORT IMaterial
{
 public:
  virtual ~IMaterial() = default;

  // Material identification
  virtual UUID
  getMaterialId() const = 0;

  virtual String
  getName() const = 0;

  // Shader association
  virtual SPtr<IShader>
  getVertexShader() const = 0;

  virtual SPtr<IShader>
  getFragmentShader() const = 0;

  virtual SPtr<IShader>
  getShader(ShaderStage stage) const = 0;

  virtual bool
  isStageSupported(ShaderStage stage) const = 0;

  virtual MaterialParameter
  getParameter(const String& name) const = 0;

  virtual Vector<String>
  getParameterNames() const = 0;

  virtual SPtr<IPipeline>
  createPipeline(const PipelineCreateInfo& baseInfo) const = 0;

  virtual bool
  isValid() const = 0;

  virtual bool
  isCompatibleWith(const VertexLayout& layout) const = 0;

 protected:
  class MaterialManager;

  virtual void
  setShader(SPtr<IShader> shader, ShaderStage stage) = 0;
};
} // namespace chEngineSDK
