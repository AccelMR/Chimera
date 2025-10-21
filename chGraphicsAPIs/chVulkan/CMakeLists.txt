# Requiere CMake 3.16+ para target_sources con archivos a�adidos condicionalmente
cmake_minimum_required(VERSION 3.16)

# (Opcional) si ya est� definido en el superproyecto, puedes omitirlo:
# project(chVulkan LANGUAGES CXX)

# ---------- Opciones ----------
option(CH_VULKAN_IMGUI "Enable ImGui support in chVulkan plugin" ON)

# ---------- Fuentes ----------
file(GLOB SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

set(CMAKE_DEBUG_POSTFIX "d")
add_library(chVulkan SHARED ${SOURCE_FILES})

set_target_properties(chVulkan PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME "chVulkan"
    PREFIX ""
)

# ---------- Funci�n para a�adir subcarpetas como includes (por-target) ----------
function(target_include_subfolders tgt base_dir)
    file(GLOB _children LIST_DIRECTORIES true "${base_dir}/*")
    foreach(_child IN LISTS _children)
        if(IS_DIRECTORY "${_child}")
            target_include_directories(${tgt} PRIVATE "${_child}")
        endif()
    endforeach()
endfunction()

# Estructura de includes equivalente a tu macro original
target_include_subfolders(chVulkan ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_subfolders(chVulkan ${CMAKE_CURRENT_SOURCE_DIR}/../../chUtilities/src)
target_include_subfolders(chVulkan ${CMAKE_CURRENT_SOURCE_DIR}/../../chCore/src)
target_include_subfolders(chVulkan ${CMAKE_CURRENT_SOURCE_DIR}/../../chCore/src/Platform)

# ---------- ImGui (solo si est� habilitado) ----------
if(CH_VULKAN_IMGUI)
    set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/Externals/imgui)
    message("ImGui directory: ${IMGUI_DIR}")
    message(STATUS "chVulkan: ImGui support ENABLED")

    target_sources(chVulkan PRIVATE )

    target_compile_definitions(chVulkan PRIVATE
        CH_VULKAN_IMGUI=1
        IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        IMGUI_DISABLE_OBSOLETE_KEYIO
    )
else()
    message(STATUS "chVulkan: ImGui support DISABLED")
    target_compile_definitions(chVulkan PRIVATE CH_VULKAN_IMGUI=0)
endif()

# ---------- SDL3 flag (sin cambiar tu l�gica) ----------
if(CH_SDL3_ENABLED)
    target_compile_definitions(chVulkan PRIVATE CH_SDL3_ENABLED)
    message(STATUS "SDL3 support enabled in chVulkan")
    target_link_libraries(chVulkan PRIVATE SDL3::SDL3)
else()
    target_compile_definitions(chVulkan PRIVATE CH_SDL3_ENABLED=0)
    message(STATUS "SDL3 support disabled in chVulkan")
endif()

# ---------- Vulkan (com�n a todas las plataformas) ----------
find_package(Vulkan REQUIRED)                # Exporta el target Vulkan::Vulkan
target_link_libraries(chVulkan PRIVATE Vulkan::Vulkan chImgui)

# ---------- Plataforma: Windows ----------
if(WIN32)
    # Define para surface Win32
    target_compile_definitions(chVulkan PRIVATE VK_USE_PLATFORM_WIN32_KHR IMGUI_API=__declspec(dllimport))

    # Si tu c�digo crea ventanas Win32 o usa funciones del SDK, enlaza estas (seguras/no-controversiales)
    target_link_libraries(chVulkan PRIVATE user32 gdi32 shell32)

# ---------- Plataforma: Linux (X11/XCB) ----------
elseif(UNIX AND NOT APPLE)
    # XCB v�a pkg-config (m�s robusto que rutas hardcodeadas)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XCB REQUIRED xcb)

    if(XCB_FOUND)
        message(STATUS "Found XCB: ${XCB_LINK_LIBRARIES}")
        target_include_directories(chVulkan PRIVATE ${XCB_INCLUDE_DIRS})
        target_link_libraries(chVulkan PRIVATE ${XCB_LINK_LIBRARIES})
    else()
        message(FATAL_ERROR "XCB not found. En Arch: sudo pacman -S libxcb")
    endif()

    # X11 (si lo usas junto con XCB)
    find_package(X11 REQUIRED)  # m�dulo cl�sico de CMake
    if(X11_FOUND)
        target_include_directories(chVulkan PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(chVulkan PRIVATE ${X11_LIBRARIES})
    endif()

    # Defines de plataformas Vulkan que ya usabas
    target_compile_definitions(chVulkan PRIVATE
        VK_USE_PLATFORM_XCB_KHR
        VK_USE_PLATFORM_XLIB_KHR
    )
endif()

# ---------- Enlaces comunes ----------
target_link_libraries(chVulkan PRIVATE chCore chUtilities)

# ---------- Mensajes de configuraci�n ----------
if(CH_VULKAN_IMGUI)
    message(STATUS "chVulkan configured with ImGui Vulkan backend")
    message(STATUS "ImGui directory: ${IMGUI_DIR}")
else()
    message(STATUS "chVulkan configured WITHOUT ImGui support")
endif()
message(STATUS "Vulkan found: ${Vulkan_FOUND}")
