/************************************************************************/
/**
 * @file chUsing.h
 * @author AccelMR
 * @date 2022/06/21
 * @brief Macros to check feature flags with USING(X).
 *
 * A flag is defined as IN_USE or NOT_IN_USE, never left undefined, so USING() of a flag
 * that was never defined is a build error instead of silently reading as false.
 */
/************************************************************************/
#pragma once

#if !defined(USING)
#define IN_USE &&
#define NOT_IN_USE &&!
#define USE_IF(x) &&((x)?1:0)&&
#define USING(x) (1 x 1)
#endif // #if !defined(USING)
