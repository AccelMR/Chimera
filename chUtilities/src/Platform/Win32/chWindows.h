/************************************************************************/
/**
 * @file chWindows.h
 * @author AccelMR
 * @date 2022/06/15
 *   Custom Windows header for Chimera engine. 
 * 
 *  NEVER include windows.h, always include this specific file so we can get
 *  rid of the unnecessary stuff. 
 */
 /************************************************************************/
 #pragma once

 /************************************************************************/
 /*
  * Includes
  */                                                                     
 /************************************************************************/
#ifdef _WINDOWS_
# pragma message ( " " )
# pragma message ( "You have included Windows.h before chMinWindows.h" )
# pragma message ( "All useless stuff from the windows headers won't be excluded !!!" )
# pragma message ( " " )
#endif // _WINDOWS_

/**
 * WIN32_LEAN_AND_MEAN excludes rarely used services from the windows header
 */
#define WIN32_LEAN_AND_MEAN


/*****************************************************************************/
/**
 * Now finally we can include the Windows Headers
 */
/*****************************************************************************/
#include <Windows.h>
