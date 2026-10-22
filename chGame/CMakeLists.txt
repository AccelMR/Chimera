# ---------- Game runtime ----------
ch_collect_sources(GAME_SOURCES)
ch_add_executable(chGame WINDOWED FOLDER Game SOURCES chGameMain.cpp ${GAME_SOURCES})
ch_include_subfolders(chGame PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_link_libraries(chGame PRIVATE chCore)

# The project a development build opens without -project. A packaged game has its project
# next to the executable, which wins over this.
target_compile_definitions(chGame PRIVATE CH_GAME_DEV_PROJECT="${CH_GAME_DEV_PROJECT}")

# Graphics plugins are loaded at run time, so nothing links them; building the game builds
# them too.
foreach(PLUGIN chVulkan chDX12)
  if(TARGET ${PLUGIN})
    add_dependencies(chGame ${PLUGIN})
  endif()
endforeach()
