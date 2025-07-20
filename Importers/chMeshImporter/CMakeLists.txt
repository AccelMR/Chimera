file(GLOB SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)


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

# Configuración de Assimp
set(ASSIMP_BUILD_STATIC_LIB ON CACHE BOOL "Build assimp as static library" FORCE)
set(ASSIMP_BUILD_SHARED_LIB OFF CACHE BOOL "Don't build assimp as shared library" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable assimp tests" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "Disable assimp samples" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable assimp tools" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "Disable assimp install" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable assimp export functionality" FORCE)
set(BUILD_SHARED_LIBS_SAVED ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
message(STATUS "Assimp configured for static linking")
add_subdirectory(externals/assimp)

add_library(chMeshImporter SHARED ${SOURCE_FILES})

# Configurar la salida de Assimp
set_target_properties(assimp PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
set_target_properties(chMeshImporter PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/Importers
    OUTPUT_NAME "chMeshImporter"
    PREFIX ""
    #want to add a postfix if we are in debug mode
    DEBUG_POSTFIX "d"
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
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chCore/src)

target_include_directories(chMeshImporter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/assimp/include
    )
target_link_libraries(chMeshImporter PUBLIC chUtilities chCore assimp::assimp)

target_compile_definitions(chMeshImporter PRIVATE
    CH_EDITOR_ENABLED=${CH_EDITOR_ENABLED}
    CH_SDL3_ENABLED=${CH_SDL3_ENABLED}
    CH_IMPORTERS_ENABLED=${CH_IMPORTERS_ENABLED}
    ASSIMP_BUILD_STATIC_LIB
    ASSIMP_BUILD_NO_EXPORT
)
