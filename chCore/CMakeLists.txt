file(GLOB
    SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/crosswindow/*.h
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

add_subdirectory(externals/crosswindow)

# Configurar la salida de CrossWindow
set_target_properties(CrossWindow PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Configuración de Assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
add_subdirectory(externals/assimp)

# Configurar la salida de Assimp
set_target_properties(assimp PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

add_library(chCore SHARED ${SOURCE_FILES})

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

add_compile_definitions(CH_CORE_EXPORTS CH_CROSS_WINDOW)

INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics)
INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/../chUtilities/src)

target_include_directories(chCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/crosswindow/src
    ${CMAKE_CURRENT_SOURCE_DIR}/externals
)

file(GLOB_RECURSE CROSSWIN_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/CrossWindow/*.cpp
)
target_sources(chCore PRIVATE ${CROSSWIN_FILES})

if(UNIX AND NOT APPLE)
    target_link_libraries(chCore PUBLIC chUtilities CrossWindow xcb xcb-keysyms assimp)
else()
    target_link_libraries(chCore PUBLIC chUtilities CrossWindow assimp)
endif()

target_include_directories(chCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../chUtilities/src/Platform/SDL
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/crosswindow/src
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/CrossWindow
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/assimp/include
)