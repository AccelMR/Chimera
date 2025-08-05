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

#include "chColor.h"
#include "chMatrix4.h"
#include "chLinearColor.h"
#include "chVector2.h"
#include "chVector3.h"
#include "chVector4.h"

namespace chEngineSDK {
class UUID;

struct MaterialParameter{
  enum class Type {
    Float,
    Vector2,
    Vector3,
    Vector4,
    Matrix4,
    TextureView,
    Color,
    LinearColor,
    Sampler,
  };
  Type type;

  Variant<
    float,
    Vector2,
    Vector3,
    Vector4,
    Matrix4,
    SPtr<ITextureView>,
    LinearColor,
    Color,
    SPtr<ISampler>,
    std::monostate> value;

  MaterialParameter() : type(Type::Float), value(0.0f) {}

  explicit MaterialParameter(float val) : type(Type::Float), value(val) {}
  explicit MaterialParameter(const Vector2& val) : type(Type::Vector2), value(val) {}
  explicit MaterialParameter(const Vector3& val) : type(Type::Vector3), value(val) {}
  explicit MaterialParameter(const Vector4& val) : type(Type::Vector4), value(val) {}
  explicit MaterialParameter(const Matrix4& val) : type(Type::Matrix4), value(val) {}
  explicit MaterialParameter(SPtr<ITextureView> val) : type(Type::TextureView), value(std::move(val)) {}
  explicit MaterialParameter(const LinearColor& val) : type(Type::LinearColor), value(val) {}
  explicit MaterialParameter(const Color& val) : type(Type::Color), value(val) {}
  explicit MaterialParameter(SPtr<ISampler> val) : type(Type::Sampler), value(std::move(val)) {}

  /**
   * @brief Get the value of the parameter as a specific type
   * @tparam T The type to retrieve
   * @return The value of the parameter as type T
   */
  template<typename T>
  T getValue() const noexcept{
    if (!std::holds_alternative<T>(value)) {
      return T(); // Return default value if type does not match
    }
    return std::get<T>(value);
  }

  /**
   * @brief Get the value of the parameter as a float
   * @return The value of the parameter as a float
   */
  FORCEINLINE float
  getFloat() const {
    return getValue<float>();
  }

  /**
   * @brief Get the value of the parameter as a Vector2
   * @return The value of the parameter as a Vector2
   */
  FORCEINLINE Vector2
  getVector2() const {
    return getValue<Vector2>();
  }

  /**
   * @brief Get the value of the parameter as a Vector3
   * @return The value of the parameter as a Vector3
   */
  FORCEINLINE Vector3
  getVector3() const {
    return getValue<Vector3>();
  }

  /**
   * @brief Get the value of the parameter as a Vector4
   * @return The value of the parameter as a Vector4
   */
  FORCEINLINE Vector4
  getVector4() const {
    return getValue<Vector4>();
  }

  /**
   * @brief Get the value of the parameter as a Matrix4
   * @return The value of the parameter as a Matrix4
   */
  FORCEINLINE Matrix4
  getMatrix4() const {
    return getValue<Matrix4>();
  }

  /**
   * @brief Get the value of the parameter as a TextureView
   * @return The value of the parameter as a TextureView
   */
  FORCEINLINE SPtr<ITextureView>
  getTextureView() const {
    return getValue<SPtr<ITextureView>>();
  }

  /**
   * @brief Get the value of the parameter as a LinearColor
   * @return The value of the parameter as a LinearColor
   */
  FORCEINLINE LinearColor
  getLinearColor() const {
    return getValue<LinearColor>();
  }

  /**
   * @brief Get the value of the parameter as a Color
   * @return The value of the parameter as a Color
   */
  FORCEINLINE Color
  getColor() const {
    return getValue<Color>();
  }

  /**
   * @brief Get the value of the parameter as a Sampler
   * @return The value of the parameter as a Sampler
   */
  FORCEINLINE SPtr<ISampler>
  getSampler() const {
    return getValue<SPtr<ISampler>>();
  }

  /**
   * @brief Get the type of the parameter
   * @return The type of the parameter
   */
  FORCEINLINE Type
  getType() const {
    return type;
  }

  /**
   * @brief Check if the parameter is of a specific type
   * @tparam T The type to check against
   * @return true if the parameter is of type T, false otherwise
   */
  template<typename T>
  FORCEINLINE bool
  isType() const noexcept {
    return std::holds_alternative<T>(value);
  }

  /**
   * @brief Check validation of the parameter
   * @return true if the parameter is valid, false otherwise
   */
  FORCEINLINE bool
  isValid() const noexcept{
    return !std::holds_alternative<std::monostate>(value);
  }
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
