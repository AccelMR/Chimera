# ---------- Editor ----------
ch_collect_sources(EDITOR_SOURCES)
ch_add_executable(chEditor WINDOWED FOLDER Editor SOURCES chEditorMain.cpp ${EDITOR_SOURCES})
ch_include_subfolders(chEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_link_libraries(chEditor PRIVATE chCore chImguiSDL3 chImGuizmo)

# Plugins are loaded at run time, so nothing links them; building the editor builds them too.
foreach(PLUGIN chVulkan chDX12 chImageCodec chMeshCodec)
  if(TARGET ${PLUGIN})
    add_dependencies(chEditor ${PLUGIN})
  endif()
endforeach()

ch_add_shaders(chEditorShaders ${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders)
set_target_properties(chEditorShaders PROPERTIES FOLDER Editor)
add_dependencies(chEditor chEditorShaders)

if(CH_EDITOR_DEV_PROJECT)
  set_target_properties(chEditor PROPERTIES
    VS_DEBUGGER_COMMAND_ARGUMENTS "-project=${CH_EDITOR_DEV_PROJECT}")
endif()
