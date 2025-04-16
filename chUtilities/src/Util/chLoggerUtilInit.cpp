#include "chLogger.h"

namespace chEngineSDK
{

#ifdef CH_ENABLE_UTILITITES_LOGGING
#define CH_UTILITIES_LOG_LEVEL Info
#else
#define CH_UTILITIES_LOG_LEVEL NoLogging
#endif
CH_LOG_DEFINE_CATEGORY_SHARED(Utilities, CH_UTILITIES_LOG_LEVEL);

} // namespace chEngineSDK