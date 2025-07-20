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

add_library(chImageImporter SHARED ${SOURCE_FILES})

set_target_properties(chImageImporter PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/Importers
    OUTPUT_NAME "chImageImporter"
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
INCLUDESUBFOLDERS(${CMAKE_CURRENT_SOURCE_DIR}/externals)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chUtilities/src)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chCore/src)


target_link_libraries(chImageImporter PUBLIC chUtilities chCore)

target_compile_definitions(chImageImporter PRIVATE
    CH_EDITOR_ENABLED=${CH_EDITOR_ENABLED}
    CH_SDL3_ENABLED=${CH_SDL3_ENABLED}
    CH_IMPORTERS_ENABLED=${CH_IMPORTERS_ENABLED}
)
