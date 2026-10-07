ch_add_plugin(chImageCodec FOLDER Codecs CODEC)

target_link_libraries(chImageCodec PRIVATE chCore)

# stb_image is header-only; SYSTEM keeps our warning level out of its code.
target_include_directories(chImageCodec SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Externals/stb)
