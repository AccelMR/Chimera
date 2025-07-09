file(GLOB
  SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp
)

# ImGui Source Files - Configuración específica para Chimera Engine
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui)
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp

  # Backends específicos para Chimera
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# Configuración de debug postfix
set(CMAKE_DEBUG_POSTFIX "d")

# Crear ejecutable con archivos de ImGui incluidos
add_executable(chEditor chEditorMain.cpp ${SOURCE_FILES} ${IMGUI_SOURCES})

# Configuración PIE específica para Linux y arquitectura de plugins Chimera
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Configuración de flags del compilador para ImGui + Vulkan
if(UNIX AND NOT APPLE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

  # Flags específicos para el editor con ImGui
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic -pie")

  # Permitir relocaciones dinámicas para plugins
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC")
endif()

# Configuración de RPATH para arquitectura de plugins
set_target_properties(chEditor PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib"
  BUILD_WITH_INSTALL_RPATH TRUE
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Macro para inclusión de subdirectorios (manteniendo el estilo Chimera)
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

# Inclusión de directorios del proyecto
INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chUtilities/src)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chCore/src)

# Configuración específica de ImGui para Chimera Engine
target_include_directories(chEditor PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

# Configuración de compilación específica para ImGui
target_compile_options(chEditor PRIVATE
  $<$<PLATFORM_ID:Linux>:-fPIC>
)

# Definiciones específicas para ImGui en Chimera Engine
target_compile_definitions(chEditor PRIVATE
  IMGUI_IMPL_VULKAN_USE_VOLK=0
  IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  IMGUI_DISABLE_OBSOLETE_KEYIO
  $<$<PLATFORM_ID:Linux>:VK_USE_PLATFORM_XCB_KHR>
  $<$<PLATFORM_ID:Linux>:VK_USE_PLATFORM_XLIB_KHR>
)

# Búsqueda de dependencias requeridas
find_package(SDL3 REQUIRED CONFIG)
find_package(Vulkan REQUIRED)

# Configuración específica para Linux con XCB (siguiendo el patrón del proyecto)
if(UNIX AND NOT APPLE)
  find_path(XCB_INCLUDE_DIR NAMES xcb/xcb.h PATHS /usr/include)
  find_library(XCB_LIBRARY NAMES xcb PATHS /usr/lib)

  if(XCB_INCLUDE_DIR AND XCB_LIBRARY)
    message(STATUS "Found XCB for ImGui: ${XCB_LIBRARY}")
    include_directories(${XCB_INCLUDE_DIR})
  else()
    message(WARNING "XCB not found - some ImGui features may not work")
  endif()

  # Definiciones para Vulkan/XCB (consistente con chVulkan)
  target_compile_definitions(chEditor PRIVATE
    VK_USE_PLATFORM_XCB_KHR
    VK_USE_PLATFORM_XLIB_KHR
  )
endif()

# Enlace de bibliotecas principales de Chimera
target_link_libraries(chEditor
  chCore
  chUtilities
  SDL3::SDL3
  Vulkan::Vulkan
)

# Enlace específico para Linux con soporte PIE
if(UNIX AND NOT APPLE)
  target_link_libraries(chEditor
    ${XCB_LIBRARY}
    X11
    dl
    pthread
    m  # Biblioteca matemática
  )

  # Configuración adicional para resolver símbolos de Vulkan
  target_link_options(chEditor PRIVATE
    LINKER:--export-dynamic
    LINKER:--no-undefined
  )
endif()

# Configuración de compilador optimizada para C++20
target_compile_features(chEditor PRIVATE cxx_std_20)

# Definiciones específicas para ImGui en Chimera Engine
target_compile_definitions(chEditor PRIVATE
  IMGUI_IMPL_VULKAN_USE_VOLK=0
  IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  IMGUI_DISABLE_OBSOLETE_KEYIO
)


# Mensaje de configuración exitosa
message(STATUS "chEditor configured with ImGui (Vulkan + SDL3 backends)")
message(STATUS "ImGui directory: ${IMGUI_DIR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
