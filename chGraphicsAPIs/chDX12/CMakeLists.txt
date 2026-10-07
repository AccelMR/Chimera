# ---------- D3D12 Memory Allocator ----------
set(D3D12MA_BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(Externals/D3D12MemoryAllocator SYSTEM)
ch_set_folder_for_directory(Externals/D3D12MemoryAllocator Externals/D3D12MA)

# ---------- Plugin ----------
ch_add_plugin(chDX12 FOLDER GraphicsAPIs)

# The Windows SDK ships d3d12 and dxgi; SDL gives the native window of each swap chain.
target_link_libraries(chDX12 PRIVATE chCore SDL3::SDL3 D3D12MemoryAllocator d3d12 dxgi dxguid)
# Keeps the rarely used parts of <windows.h> out of every file of the plugin.
target_compile_definitions(chDX12 PRIVATE WIN32_LEAN_AND_MEAN)
