set(CMAKE_DEBUG_POSTFIX "d")

add_executable(chCoreTest chCoreTestMain.cpp)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

target_link_libraries(
  chCoreTest
  chCore
  chUtilities
)

macro(INCLUDESUBFOLDERS curdir)
    file(GLOB children ${curdir}/*)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${child})
            include_directories(${child})
        endif()
    endforeach()
endmacro()

INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chUtilities/src)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chCore/src)

set_target_properties(chCoreTest PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
