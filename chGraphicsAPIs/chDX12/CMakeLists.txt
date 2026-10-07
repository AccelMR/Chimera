ch_add_plugin(chDX12 FOLDER GraphicsAPIs)

# The Windows SDK ships d3d12 and dxgi; SDL gives the native window of each swap chain.
target_link_libraries(chDX12 PRIVATE chCore SDL3::SDL3 d3d12 dxgi dxguid)
# Keeps the rarely used parts of <windows.h> out of every file of the plugin.
target_compile_definitions(chDX12 PRIVATE WIN32_LEAN_AND_MEAN)
