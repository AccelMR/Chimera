option(CH_EDITOR_ENABLED "Enable chEditor" ON)
option(CH_SDL3_ENABLED "Enable SDL3 support" ON)
option(CH_IMPORTERS_ENABLED "Enable SDL3 support" ON)

file(GLOB
    SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/*/*.cpp

)

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

add_library(chCore SHARED ${SOURCE_FILES})

#if linux we use the find package for SDL3
if(UNIX AND NOT APPLE)
  find_package(SDL3 REQUIRED CONFIG)
  if(NOT SDL3_FOUND)
    add_subdirectory(externals/SDL3 EXCLUDE_FROM_ALL)
    target_include_directories(chCore PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/externals/SDL3/include)
  endif()
else()
  set(SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/externals/SDL3")
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/externals/SDL3")
endif()

set_target_properties(chCore PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME "chCore"
    PREFIX ""
    INSTALL_RPATH "$ORIGIN"
)

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

target_include_directories(chCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/externals
)

file(GLOB_RECURSE SDL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/SDL/*.cpp
)
target_sources(chCore PRIVATE ${SDL_FILES})

target_link_libraries(chCore PUBLIC chUtilities SDL3::SDL3)

target_include_directories(chCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../chCore/src/Platform/SDL
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/SDL
)

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
