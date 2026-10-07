# Dear ImGui and the backends we use, as static libraries. They are third-party code, so they
# get none of our warning settings, and SYSTEM keeps their headers out of our warnings.
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Externals/imgui)

file(GLOB IMGUI_SOURCES ${IMGUI_DIR}/*.cpp ${IMGUI_DIR}/*.h)
add_library(chImgui STATIC ${IMGUI_SOURCES})
target_include_directories(chImgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
# imconfig options must be the same in the library and in every file that includes imgui.h.
target_compile_definitions(chImgui PUBLIC
  IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  IMGUI_DISABLE_OBSOLETE_KEYIO)

add_library(chImguiSDL3 STATIC ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp)
target_link_libraries(chImguiSDL3 PUBLIC chImgui SDL3::SDL3)

set(IMGUI_TARGETS chImgui chImguiSDL3)

set_target_properties(${IMGUI_TARGETS} PROPERTIES
  SYSTEM ON
  FOLDER Externals/ImGui)
