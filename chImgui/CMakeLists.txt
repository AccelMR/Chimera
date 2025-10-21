cmake_minimum_required(VERSION 3.19)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/chImgui/Externals/imgui)
message(STATUS "IMGUI_DIR = '${IMGUI_DIR}'")


add_library(chImgui SHARED
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(chImgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

if (WIN32)
    target_compile_definitions(chImgui PRIVATE IMGUI_API=__declspec(dllexport))
endif()
