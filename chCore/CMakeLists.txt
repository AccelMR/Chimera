cmake_minimum_required(VERSION 3.15)
project(chCore)

option(CH_EDITOR_ENABLED "Enable chEditor" ON)
option(CH_SDL3_ENABLED "Enable SDL3 support" ON)
option(CH_IMPORTERS_ENABLED "Enable importers support" ON)

# Recopilar archivos fuente
file(GLOB
    SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/*/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/*/*.cpp
)

set(CMAKE_DEBUG_POSTFIX "d")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

# Configurar RPATH
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/lib")
endif()

# Añadir SDL3 desde subdirectorio (siempre, sin importar el sistema)
add_subdirectory(externals/SDL3 EXCLUDE_FROM_ALL)

# Crear la biblioteca principal
add_library(chCore SHARED ${SOURCE_FILES})

set_target_properties(chCore PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME "chCore"
    PREFIX ""
    INSTALL_RPATH "$ORIGIN"
)

# Incluir recursivamente subdirectorios
macro(INCLUDESUBFOLDERS curdir)
    file(GLOB_RECURSE children LIST_DIRECTORIES true ${curdir}/*)
    foreach(child ${children})
        if(IS_DIRECTORY ${child})
            include_directories(${child})
        endif()
    endforeach()
endmacro()

INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chUtilities/src)

# Incluir directorios públicos y privados
target_include_directories(chCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/externals
)

target_include_directories(chCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../chCore/src/Platform/SDL
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/SDL
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/SDL3/include
)

# Archivos fuente específicos de SDL
file(GLOB_RECURSE SDL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/SDL/*.cpp
)
target_sources(chCore PRIVATE ${SDL_FILES})

# Enlazar librerías necesarias
target_link_libraries(chCore PUBLIC chUtilities SDL3::SDL3)

# Definiciones de compilación
set(COMPILE_DEFS "")

if(CH_EDITOR_ENABLED)
    list(APPEND COMPILE_DEFS CH_EDITOR_ENABLED)
endif()

if(CH_SDL3_ENABLED)
    list(APPEND COMPILE_DEFS CH_SDL3_ENABLED)
endif()

if(CH_IMPORTERS_ENABLED)
    list(APPEND COMPILE_DEFS CH_IMPORTERS_ENABLED)
endif()

list(APPEND COMPILE_DEFS
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    IMGUI_DISABLE_OBSOLETE_KEYIO
)

target_compile_definitions(chCore PRIVATE ${COMPILE_DEFS})
