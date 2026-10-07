# Helpers that give every Chimera target the same setup, so each module's CMakeLists only
# lists what is particular to it: its dependencies and its own defines.
#
#   ch_add_module(<name> EXPORT_DEFINE <macro> FOLDER <folder>)   engine shared library
#   ch_add_plugin(<name> FOLDER <folder> [CODEC])                  library loaded at run time
#   ch_add_executable(<name> FOLDER <folder> SOURCES <files...>)
#   ch_add_test(<name> SOURCE <file>)
#   ch_collect_sources(<out var>)       the src/ files, for targets built by hand
#
# Sources are every .cpp and .h under the target's src/ folder (found again on each build, so
# adding a file needs no manual configure).

if(UNIX AND NOT APPLE)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT CH_IPO_SUPPORTED LANGUAGES CXX)
endif()

# Adds <dir> and every folder below it to the include path, because engine includes are flat
# (#include "chLogger.h").
function(ch_include_subfolders TARGET_NAME SCOPE DIR)
  file(GLOB_RECURSE CHILDREN LIST_DIRECTORIES true "${DIR}/*")
  set(DIRS "${DIR}")
  foreach(CHILD ${CHILDREN})
    if(IS_DIRECTORY "${CHILD}")
      list(APPEND DIRS "${CHILD}")
    endif()
  endforeach()
  target_include_directories(${TARGET_NAME} ${SCOPE} ${DIRS})
endfunction()

# Puts every target created under <dir>, subfolders included, into the IDE folder <folder>.
# Used for third-party code, whose target names we do not control.
function(ch_set_folder_for_directory DIR FOLDER_NAME)
  get_property(TARGETS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(TARGET_NAME ${TARGETS})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "${FOLDER_NAME}")
  endforeach()
  get_property(SUBDIRECTORIES DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
  foreach(SUBDIRECTORY ${SUBDIRECTORIES})
    ch_set_folder_for_directory("${SUBDIRECTORY}" "${FOLDER_NAME}")
  endforeach()
endfunction()

# Settings shared by every target of ours. Warnings are errors on every compiler, and they
# are set per target so third-party code keeps its own warning level.
function(ch_target_defaults TARGET_NAME FOLDER_NAME)
  target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

  # MSVC and clang-cl take /flags (both set MSVC); GCC and clang, also clang on Windows,
  # take GCC-style flags.
  if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX
      # C4201: nameless structs and unions are accepted by every supported compiler.
      # C4251: STL members of exported classes are private, so clients never touch them
      #        across the DLL.
      # C4996: the CRT "secure" functions are MSVC only, so the standard ones are used.
      # C4324: math types are aligned for SIMD on purpose, so structs holding them get
      #        padding.
      /wd4201 /wd4251 /wd4996 /wd4324)
  else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()

  if(CH_IPO_SUPPORTED)
    set_target_properties(${TARGET_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  endif()

  set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "${FOLDER_NAME}")

  # Visual Studio shows the files in their real folders instead of "Header Files" and
  # "Source Files".
  get_target_property(SOURCES ${TARGET_NAME} SOURCES)
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
endfunction()

function(ch_collect_sources OUT_VAR)
  file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
  set(${OUT_VAR} ${SOURCES} PARENT_SCOPE)
endfunction()

# An engine shared library. Its include folders are PUBLIC, so linking it is enough to use its
# headers. EXPORT_DEFINE is the macro its export header checks (CH_CORE_EXPORTS...).
function(ch_add_module TARGET_NAME)
  cmake_parse_arguments(ARG "" "EXPORT_DEFINE;FOLDER" "" ${ARGN})
  ch_collect_sources(SOURCES)
  add_library(${TARGET_NAME} SHARED ${SOURCES})
  set_target_properties(${TARGET_NAME} PROPERTIES
    DEFINE_SYMBOL ${ARG_EXPORT_DEFINE}
    PREFIX "")
  ch_include_subfolders(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
  ch_target_defaults(${TARGET_NAME} ${ARG_FOLDER})
endfunction()

# A shared library the engine loads at run time by name, so it has no "lib" prefix on Linux.
# Codecs go to the Codecs folder, one level below the engine libraries they link to.
function(ch_add_plugin TARGET_NAME)
  cmake_parse_arguments(ARG "CODEC" "FOLDER" "" ${ARGN})
  ch_collect_sources(SOURCES)
  add_library(${TARGET_NAME} SHARED ${SOURCES})
  set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")
  if(ARG_CODEC)
    set_target_properties(${TARGET_NAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${CH_CODEC_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY "${CH_CODEC_OUTPUT_DIR}"
      INSTALL_RPATH "$ORIGIN:$ORIGIN/..")
  endif()
  ch_include_subfolders(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  ch_target_defaults(${TARGET_NAME} ${ARG_FOLDER})
endfunction()

function(ch_add_executable TARGET_NAME)
  cmake_parse_arguments(ARG "" "FOLDER" "SOURCES" ${ARGN})
  add_executable(${TARGET_NAME} ${ARG_SOURCES})
  ch_target_defaults(${TARGET_NAME} ${ARG_FOLDER})
endfunction()

# A unit test executable, also registered with CTest.
function(ch_add_test TARGET_NAME)
  cmake_parse_arguments(ARG "" "SOURCE" "" ${ARGN})
  ch_add_executable(${TARGET_NAME} FOLDER Tests SOURCES ${ARG_SOURCE})
  add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()
