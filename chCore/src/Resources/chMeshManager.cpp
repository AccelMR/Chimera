/************************************************************************/
/**
 * @file chMeshManager.cpp
 * @author AccelMR
 * @date 2025/04/19
 * @brief
 * Implementation of the MeshManager class for loading and managing mesh resources.
 */
/************************************************************************/

#include "chMeshManager.h"

#include "chFileSystem.h"
#include "chLogger.h"
#include "chMatrix4.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace chEngineSDK {
namespace MeshManagerHelpers {
/*
*/
static Matrix4
convertAssimpMatrix(const aiMatrix4x4& matrix) {
  Matrix4 result;
  std::memcpy(&result, &matrix, sizeof(Matrix4));
  return result;
  // return Matrix4(
  //   matrix.a1, matrix.a2, matrix.a3, matrix.a4,
  //   matrix.b1, matrix.b2, matrix.b3, matrix.b4,
  //   matrix.c1, matrix.c2, matrix.c3, matrix.c4,
  //   matrix.d1, matrix.d2, matrix.d3, matrix.d4
  // );
}
} // namespace MeshManagerHelpers

CH_LOG_DECLARE_STATIC(MeshSystem, All);

/*
*/
SPtr<Mesh>
MeshManager::loadMesh(const Path& meshPath, const String& meshName) {
  String name = meshName.empty() ? meshPath.getFileName() : meshName;
  
  auto it = m_meshes.find(name);
  if (it != m_meshes.end()) {
    return it->second;
  }
  
  SPtr<Model> model = loadModel(meshPath);
  if (!model || model->getMeshCount() == 0) {
    CH_LOG_ERROR(MeshSystem, "Failed to load mesh from path: {0}", meshPath.toString());
    return nullptr;
  }
  
  SPtr<Mesh> mesh = model->getMesh(0);
  
  m_meshes[name] = mesh;
  CH_LOG_DEBUG(MeshSystem, "Loaded mesh from path: {0}", meshPath.toString());
  
  return mesh;
}

/*
*/
SPtr<Model> 
MeshManager::loadModel(const Path& filePath) {
  CH_LOG_INFO(MeshSystem, "Loading model: {0}", filePath.toString());

  String modelName = filePath.getFileName();
  
  auto it = m_models.find(modelName);
  if (it != m_models.end()) {
    return it->second;
  }
  
  if (!FileSystem::isFile(filePath)) {
    CH_LOG_ERROR(MeshSystem, "File not found: {0}", filePath.toString());
    return nullptr;
  }
  
  Assimp::Importer importer;
  
  const aiScene* scene = importer.ReadFile(
    filePath.toString(),
    aiProcessPreset_TargetRealtime_MaxQuality |
    aiProcess_FlipUVs |
    aiProcess_MakeLeftHanded |
    aiProcess_PreTransformVertices
  );

    // aiProcess_Triangulate |
    // aiProcess_GenSmoothNormals |
    // aiProcess_CalcTangentSpace |
    // aiProcess_JoinIdenticalVertices |
    // aiProcess_ImproveCacheLocality |
  
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    CH_LOG_ERROR(MeshSystem, "Assimp error: {0}", importer.GetErrorString());
    return nullptr;
  }
  
  SPtr<Model> model = chMakeShared<Model>();
  
  processNodeForModel(scene->mRootNode, scene, model, Matrix4::IDENTITY);
  
  m_models[modelName] = model;
  
  return model;
}


/*
*/
void
MeshManager::unloadMesh(const WeakPtr<Mesh>& mesh) {
  //CH_NOT_IMPLEMENTED();
}

/*
*/
Vector<SPtr<Mesh>>
MeshManager::processNode(aiNode* node, const aiScene* scene) {
  Vector<SPtr<Mesh>> meshes;
  
  for (uint32 i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    SPtr<Mesh> processedMesh = processMesh(mesh, scene);
    
    if (processedMesh) {
      meshes.push_back(processedMesh);
    }
  }
  
  for (uint32 i = 0; i < node->mNumChildren; i++) {
    Vector<SPtr<Mesh>> childMeshes = processNode(node->mChildren[i], scene);
    meshes.insert(meshes.end(), childMeshes.begin(), childMeshes.end());
  }
  
  return meshes;
}

/*
*/
SPtr<Mesh>
MeshManager::processMesh(aiMesh* mesh, const aiScene* scene) {
  SPtr<Mesh> newMesh = chMakeShared<Mesh>();
  
  const bool hasPositions = mesh->HasPositions();
  const bool hasNormals = mesh->HasNormals();
  const bool hasTexCoords = mesh->HasTextureCoords(0);
  const bool hasColors = mesh->HasVertexColors(0);
  const bool hasTangents = mesh->HasTangentsAndBitangents();
  
  if (hasPositions && hasNormals && hasTexCoords) {
    Vector<VertexNormalTexCoord> vertices(mesh->mNumVertices);
    
    for (uint32 i = 0; i < mesh->mNumVertices; ++i) {
      vertices[i].position =  {
        mesh->mVertices[i].z,
        mesh->mVertices[i].x,
        mesh->mVertices[i].y
        //mesh->mVertices[i].x,
        //mesh->mVertices[i].y,
        //mesh->mVertices[i].z
      };
      vertices[i].normal = {
        mesh->mNormals[i].z,
        mesh->mNormals[i].x,
        mesh->mNormals[i].y
        // mesh->mNormals[i].x,
        // mesh->mNormals[i].y,
        // mesh->mNormals[i].z
      };
      
      if (hasTexCoords) {
        vertices[i].texCoord = { 
          mesh->mTextureCoords[0][i].x, 
          1.0f - mesh->mTextureCoords[0][i].y
        };
      } 
      else {
        CH_LOG_WARNING(MeshSystem, "Mesh does not have texture coordinates");
        vertices[i].texCoord = { 0.0f, 0.0f };
      }
    }
    
    newMesh->setVertexData(std::move(vertices));
  }
  else if (hasPositions && hasColors) {
    Vector<VertexPosColor> vertices(mesh->mNumVertices);
    
    for (uint32 i = 0; i < mesh->mNumVertices; ++i) {
      vertices[i].position = { 
        mesh->mVertices[i].x, 
        mesh->mVertices[i].y, 
        mesh->mVertices[i].z 
      };
      
      if (hasColors) {
        vertices[i].color = { 
          mesh->mColors[0][i].r, 
          mesh->mColors[0][i].g, 
          mesh->mColors[0][i].b, 
          mesh->mColors[0][i].a 
        };
      } 
      else {
        vertices[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
      }
    }
    
    newMesh->setVertexData(std::move(vertices));
  }
  else if (hasPositions) {
    CH_LOG_WARNING(MeshSystem, "Mesh does not have color data, using default color");
    Vector<VertexPosColor> vertices(mesh->mNumVertices);
    
    for (uint32 i = 0; i < mesh->mNumVertices; ++i) {
      vertices[i].position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
      
      // Assign default color if no color data is available
      vertices[i].color = { 0.7f, 0.7f, 0.7f, 1.0f };
    }
    
    newMesh->setVertexData(std::move(vertices));
  }
  else {
    CH_LOG_ERROR(MeshSystem, "Mesh does not have position data");
    return nullptr;
  }
  
  if (mesh->mMaterialIndex >= 0) {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    
    // TODO: Process material properties
    
    aiString texturePath;
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
      const String diffuseTexPath = texturePath.C_Str();
      CH_LOG_INFO(MeshSystem, "Found diffuse texture: {0}", diffuseTexPath);
    }
  }
  
  // Procesar índices
  if (mesh->HasFaces()) {
    const uint32 numIndices = mesh->mNumFaces * 3;

    // If the number of vertices is greater than 65535, use uint32 indices
    // 65535 = 2^16 - 1 which is the max value for uint16
    if (mesh->mNumVertices > std::numeric_limits<uint16>::max()) {
      Vector<uint32> indices(numIndices);
      uint32 index = 0;
      for (uint32 i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        for (uint32 j = 0; j < face.mNumIndices; j++) {
          indices[index++] = face.mIndices[j];
        }
      }
      newMesh->setIndexData(indices);
    } 
    else {
      Vector<uint16> indices(numIndices);
      uint32 index = 0;
      for (uint32 i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        for (uint32 j = 0; j < face.mNumIndices; j++) {
          indices[index++] = static_cast<uint16>(face.mIndices[j]);
        }
      }
      newMesh->setIndexData(indices);
    }
  }
  
  // AABox bounds = newMesh->calculateBounds();
  // CH_LOG_DEBUG(MeshSystem, "Mesh bounds: Min({0}, {1}, {2}), Max({3}, {4}, {5})",
  //              bounds.minPoint.x, bounds.minPoint.y, bounds.minPoint.z,
  //              bounds.maxPoint.x, bounds.maxPoint.y, bounds.maxPoint.z);
  
  return newMesh;
}

/*
*/
void 
MeshManager::processNodeForModel(aiNode* node, 
                                const aiScene* scene, 
                                SPtr<Model> model,
                                const Matrix4& parentTransform) {
  Matrix4 nodeTransform = MeshManagerHelpers::convertAssimpMatrix(node->mTransformation);
  Matrix4 globalTransform = nodeTransform;
  
  for (uint32 i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    SPtr<Mesh> processedMesh = processMesh(mesh, scene);
    
    if (processedMesh) {
      uint32 meshIndex = model->addMesh(processedMesh);
      
      model->setMeshTransform(meshIndex, globalTransform);
    }
  }
  
  for (uint32 i = 0; i < node->mNumChildren; i++) {
    processNodeForModel(node->mChildren[i], scene, model, globalTransform);
  }
}
} // namespace chEngineSDK
