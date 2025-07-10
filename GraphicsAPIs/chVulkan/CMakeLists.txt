# CMake option to control ImGui integration (default: ON)
option(CH_VULKAN_IMGUI "Enable ImGui support in chVulkan plugin" ON)

file(GLOB SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

# ImGui Vulkan backend source files (only if enabled)
set(IMGUI_VULKAN_SOURCES "")
if(CH_VULKAN_IMGUI)
    set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/Externals/imgui)
    set(IMGUI_VULKAN_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    )
    message(STATUS "chVulkan: ImGui support ENABLED")
else()
    message(STATUS "chVulkan: ImGui support DISABLED")
endif()

set(CMAKE_DEBUG_POSTFIX "d")
add_library(chVulkan SHARED ${SOURCE_FILES} ${IMGUI_VULKAN_SOURCES})

set_target_properties(chVulkan PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME "chVulkan"
    PREFIX "")

macro(INCLUDESUBFOLDERS curdir)
    file(GLOB children ${curdir}/*)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${child})
            include_directories(${child})
        endif()
    endforeach()
endmacro()

INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/../../chUtilities/src)
INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/../../chCore/src)
INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/../../chCore/src/Platform)

# ImGui include directories (only if enabled)
if(CH_VULKAN_IMGUI)
    target_include_directories(chVulkan PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
endif()

# XCB configuration for Linux
if(UNIX AND NOT APPLE)
    # Manual XCB search (Arch Linux compatible solution)
    find_path(XCB_INCLUDE_DIR NAMES xcb/xcb.h PATHS /usr/include)
    find_library(XCB_LIBRARY NAMES xcb PATHS /usr/lib)

    if(XCB_INCLUDE_DIR AND XCB_LIBRARY)
        message(STATUS "Found XCB: ${XCB_LIBRARY}")
        include_directories(${XCB_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "XCB libraries not found! Install with: sudo pacman -S libxcb")
    endif()

    # Vulkan search
    find_package(Vulkan REQUIRED)
    if(Vulkan_FOUND)
        message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")
        include_directories(${Vulkan_INCLUDE_DIRS})
    endif()

    # Necessary definitions for Vulkan/XCB
    target_compile_definitions(chVulkan PRIVATE
        VK_USE_PLATFORM_XCB_KHR
        VK_USE_PLATFORM_XLIB_KHR
    )
endif()

# ImGui Vulkan specific definitions (only if enabled)
if(CH_VULKAN_IMGUI)
    target_compile_definitions(chVulkan PRIVATE
        CH_VULKAN_IMGUI=1
        IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        IMGUI_DISABLE_OBSOLETE_KEYIO
    )
else()
    target_compile_definitions(chVulkan PRIVATE
        CH_VULKAN_IMGUI=0
    )
endif()

# Common library linking
target_link_libraries(chVulkan PRIVATE chCore chUtilities)

# Linux specific linking
if(UNIX AND NOT APPLE)
    target_link_libraries(chVulkan PRIVATE
        ${XCB_LIBRARY}
        ${Vulkan_LIBRARIES}
        X11
    )
endif()

# Configuration success message
if(CH_VULKAN_IMGUI)
    message(STATUS "chVulkan configured with ImGui Vulkan backend")
    message(STATUS "ImGui directory: ${IMGUI_DIR}")
else()
    message(STATUS "chVulkan configured WITHOUT ImGui support")
endif()
message(STATUS "Vulkan found: ${Vulkan_FOUND}")
