/************************************************************************/
/**
 * @file chShader.h
 * @author AccelMR
 * @date 2022/09/22
 *    Shader resource wrapper that hold an actual graphic shader information.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

namespace chEngineSDK{

class CH_CORE_EXPORT Shader
{
 public:
 /*
  *   Default constructor
  */
  Shader() = default;

 /*
  *   Default destructor
  */
  virtual ~Shader() = default;

 /*
  *   Get the name of the shader.
  */
  FORCEINLINE const String&
  getEntryPoint() const {
    return m_entryPoint;
  }

 protected:
  String m_entryPoint;
};
}
