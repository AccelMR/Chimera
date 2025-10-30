/************************************************************************/
/**
 * @file chGameObject.cpp
 * @author AccelMR
 * @date 2025/10/29
 * @brief
 *  GameObject class representing an entity in the scene.
 */
/************************************************************************/
#include "chGameObject.h"

namespace chEngineSDK {
/*
*/
GameObject::GameObject(const String& name)
 : Object(name) {
}

void
GameObject::addChild(SPtr<GameObject> child) {
  m_children.push_back(child);
}

} // namespace chEngineSDK
