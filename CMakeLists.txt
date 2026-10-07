cmake_minimum_required(VERSION 3.20)

project ("Chimera")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
endif()
set(CH_CODEC_OUTPUT_DIR "${CH_OUTPUT_DIR}/Codecs")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CH_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CH_OUTPUT_DIR})

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

# add warning to highest warning level only for linux, on windows we have to use /W4 or /Wall
add_library(project_warnings INTERFACE)
if (MSVC)
  target_compile_options(project_warnings INTERFACE /W4 /WX)
else()
  target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Toggle to include chImgui subdirectory
option(CH_IMGUI_ENABLED "Enable chImgui" ON)

# Configuración específica para Linux
if(UNIX AND NOT APPLE)
  # Configuración global del compilador
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

  # Configuración de linker
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fPIC")

  # Configuración para ejecutables con plugins
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic -Wl,--export-dynamic")

  message(STATUS "PIE configuration enabled for Chimera Engine plugins")
endif()

# Configuración específica para C++20 en todo el proyecto
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configuración de build type si no está especificado
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# CMake's default Release flags already optimize and define NDEBUG for every compiler.
if(UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
endif()

# Flags follow the compiler's command line style, not the platform: clang on Windows takes
# GCC-style flags, while MSVC and clang-cl take /flags (both set MSVC).
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

if(MSVC)
  # C4201: nameless structs and unions are accepted by every supported compiler.
  # C4251: STL members of exported classes are private, so clients never touch them across the DLL.
  # C4996: the CRT "secure" functions are MSVC only, so the standard ones are used instead.
  # C4324: math types are aligned for SIMD on purpose, so structs holding them get padding.
  add_compile_options(/wd4201 /wd4251 /wd4996 /wd4324)
endif()

if(WIN32)
  # Windows.h defines min and max as macros, which break std::min, std::max and
  # numeric_limits<T>::max(). Defining it here covers every include order.
  add_compile_definitions(NOMINMAX)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/chEditor")
  set(SDL_SHARED ON  CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS        OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES     OFF CACHE BOOL "" FORCE)

  set(SDL_LIBC ON  CACHE BOOL "" FORCE)
  # Windows has no separate math library; with clang there SDL would try to link m.lib.
  if(UNIX)
    set(HAVE_LIBM   ON  CACHE BOOL "" FORCE)
    set(LIBM m)
  endif()

  add_subdirectory(Externals/SDL3)
  set_target_properties(SDL3-shared PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CH_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CH_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CH_OUTPUT_DIR}
    POSITION_INDEPENDENT_CODE ON  # Add this line
  )

  set(CH_EDITOR_ENABLED ON CACHE BOOL "Enable chEditor" FORCE)
  set(CH_SDL3_ENABLED ON CACHE BOOL "Enable SDL3 support" FORCE)
  set(CH_CODECS_ENABLED ON CACHE BOOL "Enable codecs" FORCE)
  set(CH_VULKAN_IMGUI ON CACHE BOOL "Enable ImGui" FORCE)

endif()

if(CH_IMGUI_ENABLED)
  if(EXISTS "${CMAKE_SOURCE_DIR}/chImgui")
    add_subdirectory(chImgui)
  else()
    message(WARNING "CH_IMGUI_ENABLED is ON but ${CMAKE_SOURCE_DIR}/chImgui not found")
  endif()
endif()

add_subdirectory(chUtilities)

##Ad if exist depending on the stream
if(EXISTS "${CMAKE_SOURCE_DIR}/chUtilitiesTest")
  message("Directory exists.")
  add_subdirectory(chUtilitiesTest)
endif()

include(cmake/chShaders.cmake)

if(EXISTS "${CMAKE_SOURCE_DIR}/chCore")
  message("Directory exists.")
  add_subdirectory(chCore)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/chCoreTest")
  add_subdirectory(chCoreTest)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/chGraphicsAPIs/chVulkan")
  add_subdirectory(chGraphicsAPIs/chVulkan)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/chCodecs")
  message("Directory exists.")
  add_subdirectory(chCodecs)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/chEditor")
  message("Directory exists.")
  add_subdirectory(chEditor)
endif()
