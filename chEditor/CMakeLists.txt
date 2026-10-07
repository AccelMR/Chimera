# ---------- nativefiledialog-extended ----------
add_subdirectory(Externals/nativefiledialog-extended SYSTEM)
ch_set_folder_for_directory(Externals/nativefiledialog-extended Externals/nfd)

# ---------- Editor ----------
ch_collect_sources(EDITOR_SOURCES)
ch_add_executable(chEditor FOLDER Editor SOURCES chEditorMain.cpp ${EDITOR_SOURCES})
ch_include_subfolders(chEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chEditor PRIVATE chCore chImguiSDL3 nfd)

# Plugins are loaded at run time, so nothing links them; building the editor builds them too.
foreach(PLUGIN chVulkan chImageCodec chMeshCodec)
  if(TARGET ${PLUGIN})
    add_dependencies(chEditor ${PLUGIN})
  endif()
endforeach()

ch_add_shaders(chEditorShaders ${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders)
set_target_properties(chEditorShaders PROPERTIES FOLDER Editor)
add_dependencies(chEditor chEditorShaders)
