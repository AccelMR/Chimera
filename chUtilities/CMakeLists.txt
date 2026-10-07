ch_add_module(chUtilities EXPORT_DEFINE CH_UTILITY_EXPORTS FOLDER Engine)

if(WIN32)
  # <windows.h> defines min and max as macros, which break std::min, std::max and
  # numeric_limits<T>::max(). PUBLIC, so it covers every module and include order.
  target_compile_definitions(chUtilities PUBLIC NOMINMAX)
endif()
