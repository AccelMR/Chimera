/************************************************************************/
/**
 * @file chScene.cpp
 * @author AccelMR
 * @date 2025/10/28
 * @brief
 *  Scene class containing a hierarchy of GameObjects.
 */
/************************************************************************/
#include "chScene.h"

namespace chEngineSDK {
/*
*/
Scene::Scene(const String& name, UUID id)
 : Object(name, id) {
  SPtr<GameObject> rootObject = chMakeShared<GameObject>("Root");
  m_rootGameObjects.push_back(rootObject);
  m_gameObjectsMap[rootObject->getName()] = rootObject;
}

/*
*/
SPtr<GameObject>
Scene::createGameObject(const String& name, SPtr<GameObject> parent) {
  SPtr<GameObject> newObject = chMakeShared<GameObject>(name);
  if (parent) {
    parent->addChild(newObject);
  }
  else {
    m_rootGameObjects.push_back(newObject);
  }
  m_gameObjectsMap[name] = newObject;
  return newObject;
}

} // namespace chEngineSDK
