/************************************************************************/
/**
 * @file chModel.h
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 *  Model class that contains multiple meshes.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chMesh.h"
#include "chMatrix4.h"

namespace chEngineSDK {

/**
 * Class that represents a 3D model composed of multiple meshes.
 */
class CH_CORE_EXPORT Model
{
 public:
  /**
   * Default constructor
   */
  Model() = default;
  
  /**
   * Destructor
   */
  ~Model() = default;

  /**
   * Add a mesh to the model
   * 
   * @param mesh Shared pointer to the mesh to add
   * @return Index of the added mesh
   */
  uint32 addMesh(SPtr<Mesh> mesh);
  
  /**
   * Get the number of meshes in the model
   * 
   * @return Number of meshes
   */
  NODISCARD FORCEINLINE uint32 
  getMeshCount() const { return static_cast<uint32>(m_meshes.size()); }
  
  /**
   * Get a mesh by index
   * 
   * @param index Index of the mesh to get
   * @return Shared pointer to the mesh, or nullptr if index is out of range
   */
  NODISCARD FORCEINLINE SPtr<Mesh> 
  getMesh(uint32 index) const { 
    return (index < m_meshes.size()) ? m_meshes[index] : nullptr; 
  }
  
  /**
   * Get all meshes in the model
   * 
   * @return Vector of shared pointers to meshes
   */
  NODISCARD FORCEINLINE const Vector<SPtr<Mesh>>& 
  getMeshes() const { return m_meshes; }
  
  /**
   * Set the global transform matrix for the model
   * 
   * @param transform The global transform matrix
   */
  FORCEINLINE void 
  setTransform(const Matrix4& transform) { m_globalTransform = transform; }
  
  /**
   * Get the global transform matrix for the model
   * 
   * @return The global transform matrix
   */
  NODISCARD FORCEINLINE const Matrix4& 
  getTransform() const { return m_globalTransform; }

  NODISCARD FORCEINLINE const Matrix4&
  getMeshTransform(uint32 index) const {
    return (index < m_meshTransforms.size()) ?
              m_meshTransforms[index] : Matrix4::IDENTITY;
  }

  void setMeshTransform(uint32 index, const Matrix4& transform) {
    if (index < m_meshTransforms.size()) {
      m_meshTransforms[index] = transform;
    }
  }
  
  /**
   * Calculate the bounds of the model combining all mesh bounds
   * 
   * @return SphereBoxBounds containing all meshes
   */
  NODISCARD SphereBoxBounds 
  calculateBounds() const;

 private:
  Vector<SPtr<Mesh>> m_meshes;
  Matrix4 m_globalTransform = Matrix4::IDENTITY;
  Vector<Matrix4> m_meshTransforms;
};
} // namespace chEngineSDK