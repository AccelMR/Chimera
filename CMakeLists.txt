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
cmake_dependent_option(CH_BUILD_DX12 "Build the Direct3D 12 graphics plugin" ON "WIN32" OFF)
option(CH_BUILD_TESTS "Build the unit tests" ON)
option(CH_BUILD_GAME "Build chGame, the runtime that plays a project without the editor" ON)
set(CH_GAME_DEV_PROJECT "${CMAKE_SOURCE_DIR}/Examples/BaseExample" CACHE PATH
    "Project chGame opens in a development build when no -project is given")
# Empty by default, so the editor started by hand asks for its project. Only the Visual
# Studio debugger passes it (as -project); other IDEs set it in their own launch settings.
set(CH_EDITOR_DEV_PROJECT "" CACHE PATH
    "Project the Visual Studio debugger opens chEditor with; empty shows the project browser")
option(CH_ENABLE_PROFILER "Build the profiler (CPU scopes, render stats); off for shipping" ON)
option(CH_ENABLE_DEBUG_DRAW "Build the debug lines (DebugDraw and its pass); off for shipping" ON)

# Graphics plugin a windowed application loads when -GraphicsAPI is not given. The engine
# code never names a graphics API, so the build chooses it.
if(CH_BUILD_VULKAN)
  set(CH_DEFAULT_GRAPHICS_API_INIT chVulkan)
elseif(CH_BUILD_DX12)
  set(CH_DEFAULT_GRAPHICS_API_INIT chDX12)
endif()
set(CH_DEFAULT_GRAPHICS_API "${CH_DEFAULT_GRAPHICS_API_INIT}" CACHE STRING
  "Graphics plugin loaded when -GraphicsAPI is not given")

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

# Lua runs the game scripts. It is built as C++, so a script error unwinds with an exception
# and the destructors of the engine code it crosses still run (a C build would longjmp over
# them). It is a shared library so chCore, its tests and plugins all call one copy.
# lua.c and onelua.c are the standalone interpreter; ltests.c is Lua's own test suite.
file(GLOB CH_LUA_SOURCES "${CMAKE_SOURCE_DIR}/Externals/lua/*.c")
list(FILTER CH_LUA_SOURCES EXCLUDE REGEX "/(lua|onelua|ltests)\\.c$")
set_source_files_properties(${CH_LUA_SOURCES} PROPERTIES LANGUAGE CXX)
add_library(chLua SHARED ${CH_LUA_SOURCES})
target_include_directories(chLua SYSTEM PUBLIC "${CMAKE_SOURCE_DIR}/Externals/lua")
set_target_properties(chLua PROPERTIES PREFIX "" FOLDER Externals/lua)
if(WIN32)
  target_compile_definitions(chLua PUBLIC LUA_BUILD_AS_DLL)
else()
  target_compile_definitions(chLua PRIVATE LUA_USE_POSIX)
endif()

# Only the editor uses ImGui; it draws it with its own renderer on the graphics interfaces.
if(CH_BUILD_EDITOR)
  add_subdirectory(chImgui)
endif()

# ---------- Engine ----------
add_subdirectory(chUtilities)
add_subdirectory(chCore)

if(CH_BUILD_VULKAN)
  add_subdirectory(chGraphicsAPIs/chVulkan)
endif()

if(CH_BUILD_DX12)
  add_subdirectory(chGraphicsAPIs/chDX12)
endif()

if(CH_BUILD_CODECS)
  add_subdirectory(chCodecs)
endif()

if(CH_BUILD_EDITOR)
  add_subdirectory(chEditor)
endif()

if(CH_BUILD_GAME)
  add_subdirectory(chGame)
endif()

if(CH_BUILD_TESTS)
  add_subdirectory(chUtilitiesTest)
  add_subdirectory(chCoreTest)
endif()
