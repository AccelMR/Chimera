file(GLOB SOURCE_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h)

set(CMAKE_DEBUG_POSTFIX "d")
add_library(chUtilities SHARED ${SOURCE_FILES})
set_target_properties(chUtilities PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
  OUTPUT_NAME "chUtilities"
    PREFIX "")

macro(INCLUDESUBFOLDERS curdir)
    file(GLOB children ${curdir}/*)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${child})
            include_directories(${child})
        endif()
    endforeach()
endmacro()

add_compile_definitions(CH_UTILITY_EXPORTS)

INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/src)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Perform Linux-specific actions
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")

  file(GLOB_RECURSE WIN32FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32/*.h
      )
  target_sources(chUtilities PRIVATE ${WIN32FILES})

  INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/../chUtilities/src/Platform/Win32)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Win32)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # Perform macOS-specific actions
else()
  message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()