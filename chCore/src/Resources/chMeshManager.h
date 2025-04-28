/************************************************************************/
/**
 * @file chMeshManager.h
 * @author AccelMR
 * @date 2025/04/19
 * @brief
 * MeshManager class for loading and managing mesh resources.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chModule.h"
#include "chMesh.h"
#include "chModel.h"

//Forward declarations from Assimp
class aiNode;
class aiMesh;
class aiScene;

namespace chEngineSDK {

class CH_CORE_EXPORT MeshManager : public Module<MeshManager> {
 public:
  MeshManager() = default;
  ~MeshManager() = default;

  /**
   * Load a mesh from a file
   * 
   * @param filename Path to the mesh file
   * @param meshName Name of the mesh (optional)
   * @return Pointer to the loaded mesh
   */
  SPtr<Mesh> 
  loadMesh(const Path& filename, const String& meshName = "");

  /**
   * Load a model from a file
   * 
   * @param filename Path to the model file
   * @return Vector of pointers to the loaded meshes
   */
  SPtr<Model>
  loadModel(const Path& filename);

  /**
   * Unload a mesh
   * 
   * @param mesh Pointer to the mesh to unload
   */
  void 
  unloadMesh(const WeakPtr<Mesh>& mesh);

 private:

  /**
   * Process a node in the Assimp scene
   *
   * @param node Pointer to the Assimp node
   * @param scene Pointer to the Assimp scene
   * @return Vector of pointers to the processed meshes
   */
  Vector<SPtr<Mesh>>
  processNode(aiNode* node, const aiScene* scene);

  /**
   * Process a mesh in the Assimp scene
   *
   * @param mesh Pointer to the Assimp mesh
   * @param scene Pointer to the Assimp scene
   * @return Pointer to the processed mesh
   */
  SPtr<Mesh>
  processMesh(aiMesh* mesh, const aiScene* scene);

  void
  processNodeForModel(aiNode* node, 
                      const aiScene* scene, 
                      SPtr<Model> model,
                      ModelNode* parentNode);


 private:
  UnorderedMap<String, SPtr<Mesh>> m_meshes;
  UnorderedMap<String, SPtr<Model>> m_models;
  Mutex m_mutex;
};

} // namespace chEngineSDK