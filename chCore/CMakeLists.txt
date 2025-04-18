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

add_subdirectory(externals/crosswindow)

add_library(chCore SHARED ${SOURCE_FILES})

set_target_properties(chCore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME "chCore"
    PREFIX "")

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
    target_link_libraries(chCore PUBLIC chUtilities CrossWindow xcb xcb-keysyms)
else()
    target_link_libraries(chCore chUtilities CrossWindow)
endif()


target_include_directories(chCore PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../chUtilities/src/Platform/SDL
  ${CMAKE_CURRENT_SOURCE_DIR}/externals/crosswindow/src
  ${CMAKE_CURRENT_SOURCE_DIR}/Platform/CrossWindow
)




##########################################################################
# if(WIN32)
#     file(GLOB_RECURSE WIN32FILES 
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32/*.cpp
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32/*.h
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32/*.cpp
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32/*.h
#     )
#     target_sources(chCore PRIVATE ${WIN32FILES})

#     target_include_directories(chCore PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/../chUtilities/src/Platform/Win32
#     )

#     target_link_libraries(chCore chUtilities)
# elseif(APPLE)
#     # Perform macOS-specific actions
# elseif(UNIX)

#     find_package(SDL3 REQUIRED)

#     file(GLOB_RECURSE WAYLANDFILES 
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/SDL/*.cpp
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/SDL/*.h
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/SDL/*.cpp
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/SDL/*.h
#     )
#     target_sources(chCore PRIVATE ${WAYLANDFILES})

#     target_include_directories(chCore PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/../chUtilities/src/Platform/SDL
#     )
##########################################################################