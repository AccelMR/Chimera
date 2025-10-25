file(GLOB
  SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Externals/*/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Externals/*/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/EditorAssets/*/*.h
)

#set(NFD_PORTAL OFF d BOOL "Use xdg-desktop-portal instead of GTK" FORCE)
add_subdirectory(Externals/nativefiledialog-extended)

set_target_properties(nfd PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Debug postfix configuration
set(CMAKE_DEBUG_POSTFIX "d")

# Create executable with ImGui core files only
add_executable(chEditor chEditorMain.cpp ${SOURCE_FILES})

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

target_sources(chEditor PRIVATE
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
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
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chCore/src/AssetManagment)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chCore/src/Renderer)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chEditor/Externals/nativefiledialog-extended/src)

# ImGui core configuration for Chimera Engine
target_include_directories(chEditor PRIVATE
  ${IMGUI_DIR}
)

# Compilation options for ImGui
target_compile_options(chEditor PRIVATE
  $<$<PLATFORM_ID:Linux>:-fPIC>
)

# ImGui core definitions (without Vulkan)
target_compile_definitions(chEditor PRIVATE
  CH_EDITOR_ENABLED
  CH_SDL3_ENABLED
  CH_CODECS_ENABLED
  IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  IMGUI_DISABLE_OBSOLETE_KEYIO
)


# Link main Chimera libraries
target_link_libraries(chEditor
  chCore
  chUtilities
  chImgui
  nfd
)

# C++20 compiler configuration
target_compile_features(chEditor PRIVATE cxx_std_20)

target_compile_definitions(chEditor PRIVATE CH_EDITOR_EXPORTS)

# Success configuration message
message(STATUS "chEditor configured with ImGui (SDL3 backend only)")
message(STATUS "ImGui directory: ${IMGUI_DIR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
