ch_add_plugin(chVulkan FOLDER GraphicsAPIs)

# No VK_USE_PLATFORM_* define: SDL creates the surface and lists its extensions, and those
# defines would pull <windows.h> or <X11/Xlib.h> into every file (X11 defines None, Always...).
find_package(Vulkan REQUIRED)
target_link_libraries(chVulkan PRIVATE chCore Vulkan::Vulkan SDL3::SDL3)

# Header-only library; SYSTEM keeps our warning level out of its code.
target_include_directories(chVulkan SYSTEM PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Externals/VulkanMemoryAllocator/include)
