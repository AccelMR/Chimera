cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)

project ("Chimera")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

add_subdirectory(chUtilities)
##Ad if exist depending on the stream
if(EXISTS "${CMAKE_SOURCE_DIR}/chUtilitiesTest")
  message("Directory exists.")
  add_subdirectory(chUtilitiesTest)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/chCore")
  message("Directory exists.")
  add_subdirectory(chCore)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/chCoreTest")
  message("Directory exists.")
  add_subdirectory(chCoreTest)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/GraphicModules/chDX12Graphics")
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message("Directory exists and running on Windows.")
    add_subdirectory(GraphicModules/chDX12Graphics)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message("Running on Linux. Directory not added.")
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/GraphicModules/chVulkanGraphics")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message("Directory exists and running on Linux.")
    add_subdirectory(GraphicModules/chVulkanGraphics)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message("Running on Windows. Directory not added.")
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()
