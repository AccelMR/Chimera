file(GLOB
  SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp
)

# ImGui Source Files - Solo SDL3 backend
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/Externals/imgui)
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp

  # Solo SDL3 backend (sin Vulkan)
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)

# Debug postfix configuration
set(CMAKE_DEBUG_POSTFIX "d")

# Create executable with ImGui core files only
add_executable(chEditor chEditorMain.cpp ${SOURCE_FILES} ${IMGUI_SOURCES})

# PIE configuration for Linux and Chimera plugin architecture
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Compiler flags for ImGui (without Vulkan)
if(UNIX AND NOT APPLE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

  # Editor specific flags
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic -pie")

  # Allow dynamic relocations for plugins
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC")
endif()

# RPATH configuration for plugin architecture
set_target_properties(chEditor PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib"
  BUILD_WITH_INSTALL_RPATH TRUE
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Macro for subdirectory inclusion (maintaining Chimera style)
macro(INCLUDESUBFOLDERS curdir)
  file(GLOB children ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${child})
      message(STATUS "Adding include directory: ${child}")
      include_directories(${child})
    endif()
  endforeach()
endmacro()

# Project directory inclusion
INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chUtilities/src)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chCore/src)

# ImGui core configuration for Chimera Engine
target_include_directories(chEditor PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

# Compilation options for ImGui
target_compile_options(chEditor PRIVATE
  $<$<PLATFORM_ID:Linux>:-fPIC>
)

# ImGui core definitions (without Vulkan)
target_compile_definitions(chEditor PRIVATE
  IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  IMGUI_DISABLE_OBSOLETE_KEYIO
)

# Find required dependencies (SDL3 only, no Vulkan)
find_package(SDL3 REQUIRED CONFIG)

# Link main Chimera libraries
target_link_libraries(chEditor
  chCore
  chUtilities
  SDL3::SDL3
)

# C++20 compiler configuration
target_compile_features(chEditor PRIVATE cxx_std_20)

# Success configuration message
message(STATUS "chEditor configured with ImGui (SDL3 backend only)")
message(STATUS "ImGui directory: ${IMGUI_DIR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
