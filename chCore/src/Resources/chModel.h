/************************************************************************/
/**
 * @file chModel.h
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 *  Model class with a hierarchical tree structure of nodes.
 */
/************************************************************************/
#pragma once

#include "chMatrix4.h"
#include "chMesh.h"
#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * Forward declaration of Model class
 */
class Model;

/**
 * Structure representing a node in the model hierarchy
 */
class CH_CORE_EXPORT ModelNode
{
 public:
  // Constructors/Destructor
  ModelNode(const String& nodeName, Model* ownerModel, ModelNode* parent = nullptr);
  ~ModelNode() = default;

  // Getters
  NODISCARD FORCEINLINE const String&
  getName() const { return m_name; }

  NODISCARD FORCEINLINE ModelNode*
  getParent() const { return m_parent; }

  NODISCARD FORCEINLINE const Vector<ModelNode*>&
  getChildren() const { return m_children; }

  NODISCARD FORCEINLINE const Vector<SPtr<Mesh>>&
  getMeshes() const { return m_meshes; }

  NODISCARD FORCEINLINE const Matrix4&
  getLocalTransform() const { return m_localTransform; }

  NODISCARD FORCEINLINE const Matrix4&
  getGlobalTransform() const { return m_globalTransform; }

  NODISCARD FORCEINLINE Model*
  getModel() const { return m_model; }

  NODISCARD FORCEINLINE bool
  isDirty() const { return m_dirty; }

  // Operations
  void
  addChild(ModelNode* child);

  void
  removeChild(ModelNode* child);

  void
  addMesh(SPtr<Mesh> mesh);

  void
  removeMesh(SPtr<Mesh> mesh);

  void
  setLocalTransform(const Matrix4& transform);

  void
  updateGlobalTransform();

  // Find operations
  ModelNode*
  findChildByName(const String& name);

  // Mark this node and all children as needing transform update
  void
  markDirty();

 private:
  String m_name;
  ModelNode* m_parent;
  Vector<ModelNode*> m_children;
  Vector<SPtr<Mesh>> m_meshes;

  Matrix4 m_localTransform;
  Matrix4 m_globalTransform;

  Model* m_model;
  bool m_dirty;
};

/**
 * Class that represents a 3D model composed of a hierarchy of nodes with meshes.
 */
class CH_CORE_EXPORT Model
{
 public:
  /**
   * Default constructor
   */
  Model();

  /**
   * Destructor
   */
  ~Model();

  /**
   * Create a new node in the model
   *
   * @param name Name of the node
   * @param parent Parent node (nullptr for root nodes)
   * @return Pointer to the created node
   */
  ModelNode*
  createNode(const String& name, ModelNode* parent = nullptr);

  /**
   * Create a new node in the model with a local transform
   *
   * @param name Name of the node
   * @param localTransform Local transformation matrix
   * @param parent Parent node (nullptr for root nodes)
   * @return Pointer to the created node
   */
  ModelNode*
  createNode(const String& name, const Matrix4& localTransform, ModelNode* parent = nullptr);

  /**
   * Find a node by name
   *
   * @param name Name of the node to find
   * @return Pointer to the node, or nullptr if not found
   */
  ModelNode*
  findNode(const String& name) const;

  /**
   * Get the root nodes of the model
   *
   * @return Vector of root nodes
   */
  NODISCARD FORCEINLINE const Vector<ModelNode*>&
  getRootNodes() const { return m_rootNodes; }

  /**
   * Get all nodes in the model
   *
   * @return Vector of all nodes
   */
  NODISCARD FORCEINLINE const Vector<ModelNode*>&
  getAllNodes() const { return m_allNodes; }

  /**
   * Get the number of nodes in the model
   *
   * @return Number of nodes
   */
  NODISCARD FORCEINLINE uint32
  getNodeCount() const { return static_cast<uint32>(m_allNodes.size()); }

  /**
   * Get all meshes used in the model
   *
   * @return Map of meshes and their nodes
   */
  NODISCARD FORCEINLINE const UnorderedMap<SPtr<Mesh>, Vector<ModelNode*>>&
  getMeshToNodesMap() const { return m_meshToNodesMap; }

  /**
   * Update transforms for all nodes that need updating
   */
  void
  updateTransforms();

  /**
   * Update transform for a specific node
   *
   * @param node Node to update
   * @param localTransform New local transform
   */
  void
  updateNodeTransform(ModelNode* node, const Matrix4& localTransform);

  /**
   * Calculate the bounds of the model combining all mesh bounds
   *
   * @return SphereBoxBounds containing all meshes
   */
  SphereBoxBounds
  calculateBounds() const;

  /**
   * Set the global transform matrix for the model
   *
   * @param transform The global transform matrix
   */
  void
  setTransform(const Matrix4& transform);

  /**
   * Get the global transform matrix for the model
   *
   * @return The global transform matrix
   */
  const Matrix4&
  getTransform() const { return m_globalTransform; }

  /**
   * Get the number of meshes in the model
   *
   * @return Number of meshes
   */
  void 
  registerMeshForNode(SPtr<Mesh> mesh, ModelNode* node);

  /**
   * Unregister a mesh from a node
   *
   * @param mesh Mesh to unregister
   * @param node Node to unregister the mesh from
   */
  void 
  unregisterMeshForNode(SPtr<Mesh> mesh, ModelNode* node);

 private:
  /**
   * Add a node to the model's internal structures
   */
  void
  addNodeToStructures(ModelNode* node);

  /**
   * Remove a node from the model's internal structures
   */
  void
  removeNodeFromStructures(ModelNode* node);

 private:
  /*
    * Update the transform of a node and its hierarchy
    *
    * @param node Node to update
  */
  void
  updateNodeHierarchy(ModelNode* node);

 private:
  // Root structure
  Vector<ModelNode*> m_rootNodes;

  // All nodes for easy iteration
  Vector<ModelNode*> m_allNodes;

  // Map for quick node lookups
  UnorderedMap<String, ModelNode*> m_nodeMap;

  // Map to track which nodes contain each mesh
  UnorderedMap<SPtr<Mesh>, Vector<ModelNode*>> m_meshToNodesMap;

  // Global transform for the entire model
  Matrix4 m_globalTransform;
};

} // namespace chEngineSDK