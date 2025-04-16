file(GLOB SOURCE_FILES  
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

set(CMAKE_DEBUG_POSTFIX "d")
add_library(chVulkan SHARED ${SOURCE_FILES})

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

add_compile_definitions(CH_CROSS_WINDOW)

# Configuración específica para XCB en Linux
if(UNIX AND NOT APPLE)
    # Búsqueda manual de XCB (solución compatible con Arch Linux)
    find_path(XCB_INCLUDE_DIR NAMES xcb/xcb.h PATHS /usr/include)
    find_library(XCB_LIBRARY NAMES xcb PATHS /usr/lib)

    if(XCB_INCLUDE_DIR AND XCB_LIBRARY)
        message(STATUS "Found XCB: ${XCB_LIBRARY}")
        include_directories(${XCB_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "XCB libraries not found! Install with: sudo pacman -S libxcb")
    endif()

    # Búsqueda de Vulkan
    find_package(Vulkan REQUIRED)
    if(Vulkan_FOUND)
        message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")
        include_directories(${Vulkan_INCLUDE_DIRS})
    endif()

    # Definiciones necesarias para Vulkan/XCB
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR -DVK_USE_PLATFORM_XLIB_KHR)
endif()

# Enlazado de bibliotecas comunes
target_link_libraries(chVulkan PRIVATE chCore chUtilities)

# Enlazado específico para Linux
if(UNIX AND NOT APPLE)
    target_link_libraries(chVulkan PRIVATE 
        ${XCB_LIBRARY}
        ${Vulkan_LIBRARIES}
        X11
    )
endif()