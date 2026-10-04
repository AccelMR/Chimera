add_executable(chCoreTest chCoreTestMain.cpp)

# catch.hpp is shared with chUtilitiesTest.
target_include_directories(chCoreTest PRIVATE ${CMAKE_SOURCE_DIR}/chUtilitiesTest)

target_link_libraries(chCoreTest PRIVATE chCore chUtilities)

target_compile_features(chCoreTest PRIVATE cxx_std_20)

macro(INCLUDESUBFOLDERS curdir)
  file(GLOB_RECURSE children LIST_DIRECTORIES true ${curdir}/*)
  foreach(child ${children})
    if(IS_DIRECTORY ${child})
      include_directories(${child})
    endif()
  endforeach()
endmacro()

INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chUtilities/src)
INCLUDESUBFOLDERS(${CMAKE_SOURCE_DIR}/chCore/src)

# chCore's headers change with these flags, so the tests must see the same ones chCore
# was built with.
if(CH_EDITOR_ENABLED)
  target_compile_definitions(chCoreTest PRIVATE CH_EDITOR_ENABLED)
endif()

if(CH_SDL3_ENABLED)
  target_compile_definitions(chCoreTest PRIVATE CH_SDL3_ENABLED)
endif()

if(CH_CODECS_ENABLED)
  target_compile_definitions(chCoreTest PRIVATE CH_CODECS_ENABLED)
endif()
