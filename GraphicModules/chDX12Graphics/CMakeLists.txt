file(GLOB SOURCE_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h)

add_library(chDX12Graphics SHARED ${SOURCE_FILES})
set_target_properties(chDX12Graphics PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

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

target_link_libraries(chDX12Graphics PRIVATE chCore chUtilities)

find_library(DXGI_LIB dxgi.lib)

if (DXGI_LIB)
  target_link_libraries(chDX12Graphics PRIVATE ${DXGI_LIB} d3d12 d3dcompiler)
endif()

