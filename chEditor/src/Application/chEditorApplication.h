/************************************************************************/
/**
 * @file chEditorApplication.h
 * @author AccelMR
 * @date 2025/07/07
 * @details
 *  Chimera Editor application class.
 */
/************************************************************************/
#pragma once

#include "chWindowedApplication.h"

#if USING(CH_PLATFORM_WIN32)
# if USING(CH_COMPILER_MSVC)
#   if defined( CH_STATIC_LIB )
#     define CH_EDITOR_EXPORT
#   else
#     if defined( CH_EDITOR_EXPORTS )
#       define CH_EDITOR_EXPORT __declspec( dllexport )
#     else
#       define CH_EDITOR_EXPORT __declspec( dllimport )
#     endif
#   endif
# else  //Any other Compiler
#   if defined( CH_STATIC_LIB )
#     define CH_EDITOR_EXPORT
#   else
#     if defined( CH_EDITOR_EXPORTS )
#       define CH_EDITOR_EXPORT __attribute__ ((dllexport))
#     else
#       define CH_EDITOR_EXPORT __attribute__ ((dllimport))
#     endif
#   endif
# endif
# define CH_CORE_HIDDEN
#else //Linux/Mac settings
# if defined( CH_EDITOR_EXPORTS )
#   define CH_EDITOR_EXPORT __attribute__ ((visibility ("default")))
# else
#   define CH_EDITOR_EXPORT __attribute__ ((visibility ("default")))
# endif

# define CH_CORE_HIDDEN __attribute__ ((visibility ("hidden")))
#endif

namespace chEngineSDK {
class CH_EDITOR_EXPORT EditorApplication : public WindowedApplication {
 public:
  /**
   * @brief Default constructor.
   */
  EditorApplication();

  /**
   * @brief Default destructor.
   */
  virtual ~EditorApplication();

 protected:
  NODISCARD virtual LinearColor
  getBackgroundColor() const override;

  virtual void
  onPostInitialize() override;

  virtual void
  onRender(const float deltaTime, const SPtr<ICommandBuffer>& commandBuffer) override;

 private:
  void
  initializeEditorComponents();

  void
  bindEvents();

 private:
  SPtr<IDescriptorPool> m_descriptorPool;
};
} // namespace chEngineSDK
