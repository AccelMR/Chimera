file(GLOB SOURCE_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h)

add_library(chVulkanGraphics SHARED ${SOURCE_FILES})
set_target_properties(chVulkanGraphics PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME "chVulkanGraphics"
    PREFIX "")

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

target_link_libraries(chVulkanGraphics PRIVATE chCore chUtilities)

if (UNIX)
  find_package(Vulkan REQUIRED)
  if (Vulkan_FOUND)
    target_link_libraries(chVulkanGraphics PRIVATE ${Vulkan_LIBRARIES})
    include_directories(${Vulkan_INCLUDE_DIRS})
  endif()
endif()
