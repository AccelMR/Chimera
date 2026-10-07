# 3.25: add_subdirectory(... SYSTEM) and the SYSTEM target property keep third-party headers
# out of our warnings.
cmake_minimum_required(VERSION 3.25)

project(Chimera LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------- Modules ----------
include(CMakeDependentOption)
option(CH_BUILD_EDITOR "Build the editor; also builds chCore with its editor code" ON)
# Codecs are only loaded by the editor (IAssetCodec lives inside CH_EDITOR).
cmake_dependent_option(CH_BUILD_CODECS "Build the import codecs" ON "CH_BUILD_EDITOR" OFF)
option(CH_BUILD_VULKAN "Build the Vulkan graphics plugin" ON)
option(CH_BUILD_TESTS "Build the unit tests" ON)

# ---------- Output ----------
# Every executable and plugin goes to one folder and codecs to its Codecs subfolder,
# because the engine looks for plugins next to the executable. Multi-config
# generators (Visual Studio) get one folder per configuration, chosen here with
# $<CONFIG>; by default they would add it after Codecs/ too, giving lib/Codecs/Debug
# instead of lib/Debug/Codecs.
get_property(CH_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(CH_MULTI_CONFIG)
  set(CH_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")
else()
  set(CH_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib")
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  endif()
endif()
set(CH_CODEC_OUTPUT_DIR "${CH_OUTPUT_DIR}/Codecs")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CH_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CH_OUTPUT_DIR})

# The plugin loader adds this suffix to library names in debug builds.
set(CMAKE_DEBUG_POSTFIX "d")
# Static libraries (ImGui, assimp, zlib) end up inside shared ones.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Libraries are found next to the binary that loads them, wherever the build folder is.
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(cmake/chTargets.cmake)
include(cmake/chShaders.cmake)

if(CH_BUILD_TESTS)
  enable_testing()
endif()

# ---------- Third-party ----------
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_LIBC ON CACHE BOOL "" FORCE)
# Windows has no separate math library; with clang there SDL would try to link m.lib.
if(UNIX)
  set(HAVE_LIBM ON CACHE BOOL "" FORCE)
  set(LIBM m)
endif()
add_subdirectory(Externals/SDL3 SYSTEM)
ch_set_folder_for_directory(Externals/SDL3 Externals/SDL3)

# ImGui is used by the editor and, until the editor has its own renderer, by chVulkan.
if(CH_BUILD_EDITOR)
  add_subdirectory(chImgui)
endif()

# ---------- Engine ----------
add_subdirectory(chUtilities)
add_subdirectory(chCore)

if(CH_BUILD_VULKAN)
  add_subdirectory(chGraphicsAPIs/chVulkan)
endif()

if(CH_BUILD_CODECS)
  add_subdirectory(chCodecs)
endif()

if(CH_BUILD_EDITOR)
  add_subdirectory(chEditor)
endif()

if(CH_BUILD_TESTS)
  add_subdirectory(chUtilitiesTest)
  add_subdirectory(chCoreTest)
endif()
