# ---------- assimp ----------
# Linked statically and built with its own zlib and unzip, so the codec needs no system
# packages. With the system zlib, assimp also looks for the system minizip, adds its include
# path too late for its target, and minizip-ng has no unzip.h, so Linux could not build it.
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
# Assimp turns its warnings into errors, so a newer compiler can stop the build in code that is
# not ours.
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(Externals/assimp SYSTEM)
ch_set_folder_for_directory(Externals/assimp Externals/assimp)

# ---------- Codec ----------
ch_add_plugin(chMeshCodec FOLDER Codecs CODEC)

target_link_libraries(chMeshCodec PRIVATE chCore assimp::assimp)
