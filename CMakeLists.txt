cmake_minimum_required(VERSION 3.19)

project ("Chimera")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# add warning to highest warning level only for linux, on windows we have to use /W4 or /Wall
if (WIN32)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

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

if(EXISTS "${CMAKE_SOURCE_DIR}/chEditor")
  message("Directory exists.")
  add_subdirectory(chEditor)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/GraphicsAPIs/chVulkan")
  add_subdirectory(GraphicsAPIs/chVulkan)
endif()
