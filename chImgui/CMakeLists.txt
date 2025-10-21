cmake_minimum_required(VERSION 3.19)

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/chImgui/Externals/imgui" CACHE PATH "Path to ImGui sources")
message(STATUS "IMGUI_DIR = '${IMGUI_DIR}'")

set(CMAKE_DEBUG_POSTFIX "d")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

# Configurar RPATH antes de agregar cualquier objetivo
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Para Linux, agregar $ORIGIN para ruta relativa
if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/lib")
endif()

file(GLOB IMGUI_SRC_FILES ${IMGUI_DIR}/*.cpp ${IMGUI_DIR}/*.h)

add_library(chImgui SHARED
    ${IMGUI_SRC_FILES}
)

set_target_properties(chImgui PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME "chImgui"
    PREFIX ""
    DEBUG_POSTFIX "d"
    INSTALL_RPATH "$ORIGIN"
)

target_include_directories(chImgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_compile_definitions(chImgui PRIVATE
  IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  IMGUI_DISABLE_OBSOLETE_KEYIO
)

if (WIN32)
  target_compile_definitions(chImgui PRIVATE IMGUI_BUILD_DLL)
endif()
