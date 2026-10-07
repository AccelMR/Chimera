ch_add_module(chCore EXPORT_DEFINE CH_CORE_EXPORTS FOLDER Engine)

target_link_libraries(chCore PUBLIC chUtilities SDL3::SDL3)

# chCore's headers change with these, so they are PUBLIC: every module that includes them
# sees the same ones chCore was built with.
target_compile_definitions(chCore PUBLIC
  CH_SDL3_ENABLED
  $<$<BOOL:${CH_BUILD_EDITOR}>:CH_EDITOR_ENABLED>
  $<$<BOOL:${CH_BUILD_CODECS}>:CH_CODECS_ENABLED>)
target_compile_definitions(chCore PRIVATE
  CH_DEFAULT_GRAPHICS_API="${CH_DEFAULT_GRAPHICS_API}")

ch_add_shaders(chCoreShaders ${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders)
set_target_properties(chCoreShaders PROPERTIES FOLDER Engine)
add_dependencies(chCore chCoreShaders)
