cmake_minimum_required(VERSION 3.19)

project ("Chimera")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

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

# Optimizaciones de compilador
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
  if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
  endif()
endif()

# Configuración de warnings
if(UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
elseif(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/chEditor")
  set(SDL_SHARED ON  CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS        OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES     OFF CACHE BOOL "" FORCE)

  set(SDL_LIBC ON  CACHE BOOL "" FORCE)
  set(HAVE_LIBM   ON  CACHE BOOL "" FORCE)
  set(LIBM m)

  add_subdirectory(Externals/SDL3)
  set_target_properties(SDL3-shared PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    POSITION_INDEPENDENT_CODE ON  # Add this line
  )

  set(CH_EDITOR_ENABLED ON CACHE BOOL "Enable chEditor" FORCE)
  set(CH_SDL3_ENABLED ON CACHE BOOL "Enable SDL3 support" FORCE)
  set(CH_IMPORTERS_ENABLED ON CACHE BOOL "Enable importers" FORCE)
  set(CH_VULKAN_IMGUI ON CACHE BOOL "Enable ImGui" FORCE)

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

if(EXISTS "${CMAKE_SOURCE_DIR}/GraphicsAPIs/chVulkan")
  add_subdirectory(GraphicsAPIs/chVulkan)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/Importers")
  message("Directory exists.")
  add_subdirectory(Importers)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/chEditor")
  message("Directory exists.")
  add_subdirectory(chEditor)
endif()
