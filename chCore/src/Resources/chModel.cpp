/************************************************************************/
/**
 * @file chModel.cpp
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 *  Model class implementation.
 */
/************************************************************************/

#include "chModel.h"

#include "chSphereBoxBounds.h"
#include "chVector3.h"
#include "chVector4.h"

namespace chEngineSDK {

/*
*/
uint32 
Model::addMesh(SPtr<Mesh> mesh) {
  m_meshes.push_back(mesh);
  // Ensure we have a transform for each mesh
  m_meshTransforms.resize(m_meshes.size(), Matrix4::IDENTITY);
  return static_cast<uint32>(m_meshes.size() - 1);
}

/*
*/
SphereBoxBounds 
Model::calculateBounds() const {
  if (m_meshes.empty()) {
    return SphereBoxBounds();
  }
  
  // First, collect all vertex positions from all meshes
  Vector<Vector3> allPositions;
  
  for (const auto& mesh : m_meshes) {
    // Get positions from this mesh
    Vector<Vector3> meshPositions = mesh->extractPositions();
    
    // Transform positions by global transform
    for (auto& pos : meshPositions) {
      const Vector4 transformPos =  m_globalTransform.transformPosition(pos);
      pos = Vector3(transformPos.x, transformPos.y, transformPos.z);
    }
    
    // Add to the collection
    allPositions.insert(allPositions.end(), meshPositions.begin(), meshPositions.end());
  }
  
  // Calculate bounds using all positions
  return SphereBoxBounds(allPositions);
}

} // namespace chEngineSDK