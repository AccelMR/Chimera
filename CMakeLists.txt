cmake_minimum_required(VERSION 3.19)

project ("Chimera")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

if(EXISTS "${CMAKE_SOURCE_DIR}/GraphicsAPIs/chVulkan")
  add_subdirectory(GraphicsAPIs/chVulkan)
endif()
