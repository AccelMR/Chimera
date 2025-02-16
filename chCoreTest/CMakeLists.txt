add_executable(chCoreTest chCoreTestMain.cpp)

find_package(TBB REQUIRED COMPONENTS tbb)

# Asegúrate de que todas las bibliotecas compartidas utilicen -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Establecer -Wl,-Bsymbolic para asegurar que los símbolos estáticos se resuelvan dentro de la biblioteca
#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

target_link_libraries(
  chCoreTest
  chCore
  chUtilities
  tbb
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