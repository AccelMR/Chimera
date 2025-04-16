/************************************************************************/
/**
 * @_file chException.h
 * @author AccelMR
 * @date 2022/06/12
 * @brief This _file will keep each implementation of any exception.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Base exception class. 
 *  Every new exceptions must inherit _source here and 
 *  send its own type as a hard coded string.
 *
 */
class Exception : public std::exception
{
 public:
 /*
  * @brief Default constructor
  */
  Exception() = delete;

  Exception(const char* _type, const String& _descpription, const String& _source)
    : m_typeName(_type),
      m_description(_descpription),
      m_source(_source),
      m_fileName("unknown"),
      m_line(0)
  {}

  Exception(const char* _type, 
            const String& _description, 
            const String& _source, 
            const char* _file,
            uint32 _line)
    : m_typeName(_type),
      m_description(_description),
      m_source(_source),
      m_fileName(_file),
      m_line(_line)
  {}

  Exception(const Exception& rhs)
    : m_line( rhs.m_line ),
      m_typeName( rhs.m_typeName ),
      m_description( rhs.m_description ),
      m_source( rhs.m_source ),
      m_fileName( rhs.m_fileName ),
      m_fullDescription( rhs.m_fullDescription ) 
  {}

 /*
  * @brief Default destructor
  */
  ~Exception() _NOEXCEPT = default;

  /** 
   *   Returns a string with the full description.
   * 
   * @note  The description contains the error number, the description
   *        supplied by the thrower, what routine threw the exception, and
   *        will also supply extra platform-specific information where applicable.
   * 
   * @return
   **/
  virtual const String& 
  getFullDescription() const {
    if (m_fullDescription.empty()) {
      StringStream desc;
      desc << "Chimera EXCEPTION(" << m_typeName << "): "
        << m_description << " in " << m_source;

      if (m_line > 0) {
        desc << " at " << m_fileName << " (line " << m_line << ")";
      }

      m_fullDescription = desc.str();
    }

    return m_fullDescription;
  }

  
  /**
   * @brief Overriden std::exception::what. Returns the same value as "getFullDescription".
   */
  const char* 
  what() const noexcept override {
    return getFullDescription().c_str();
  }

protected:
  String m_typeName;
  String m_description;
  String m_source;
  String m_fileName;
  uint32 m_line; 

  mutable String m_fullDescription;
};

/**
 * @brief	Exception for signaling an internal error, normally something that shouldn't
 *        have happened or wasn't anticipated by the programmers of that system.
 */
class InternalErrorException : public Exception
{
public:
  InternalErrorException(const String& inDescription,
                         const String& inSource,
                         const char* inFile,
                         long inLine)
    : Exception("InternalErrorException", inDescription, inSource, inFile, inLine) {}
};

/**
 * @brief	Exception for not implemented error, normally something that is not implemented yet.
 */
class NotImplementedException : public Exception
{
public:
  NotImplementedException(const String& inDescription,
                         const String& inSource,
                         const char* inFile,
                         long inLine)
    : Exception("NotImplementedException", inDescription, inSource, inFile, inLine) {}
};

/**
 * @brief	Exception for signaling an invalid parameters error, normally when a parameter
 *        is invalid or outside of the specified range.
 */
class RunTimeException : public Exception
{
public:
  RunTimeException(const String& inDescription,
                   const String& inSource,
                   const char* inFile,
                   long inLine)
    : Exception("RunTimeException", inDescription, inSource, inFile, inLine) {}
};

/**
 * @brief	Exception for signaling an invalid parameters error, normally when a parameter
 *        is invalid or outside of the specified range.
 */
class InvalidArgumentException : public Exception
{
public:
  InvalidArgumentException(const String& inDescription,
                           const String& inSource,
                           const char* inFile,
                           long inLine)
    : Exception("InvalidArgumentException", inDescription, inSource, inFile, inLine) {}
};


#define CH_EXCEPT(type, desc)                                                   \
{                                                                               \
static_assert((std::is_base_of<chEngineSDK::Exception, type>::value),           \
  "Invalid exception type (" #type ") for CH_EXEPT macro."                      \
  "It needs to derive from chEngineSDK::Exception.");                           \
  throw type(desc, __PRETTY_FUNCTION__, __FILE__, __LINE__);                    \
}
}

