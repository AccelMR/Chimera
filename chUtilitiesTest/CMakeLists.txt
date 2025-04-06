# Define el ejecutable para las pruebas
add_executable(chUtilitiesTest chUtilitiesMain.cpp chBitmapTest.cpp)

# Añade el include directory para Catch2 (mismo directorio que chUtilitiesMain.cpp)
target_include_directories(chUtilitiesTest PRIVATE ${CMAKE_SOURCE_DIR})

# Linkea tu biblioteca principal
target_link_libraries(
  chUtilitiesTest
  chUtilities
)

# Macro para incluir subcarpetas
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
