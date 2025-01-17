/************************************************************************/
/**
 * @file chUsing.h
 * @author AccelMR
 * @date 2022/06/21
 * @brief 
 * 
 * USING, USE_IF, and USE_IFDEF macros
 *  When writing preprocessor conditionals, one usually either
 *  only defines Symbol to indicate true and uses '#ifdef Symbol' or else
 *  defines a Symbol as 0 or 1 and then uses '#if Symbol'
 *  (and if the Symbol is not defined, the preprocessor treats it as 0).
 *
 *  The problem with these approaches is that they don't detect
 *  when a wrong undeclared Symbol is accidentally being used.
 *  Both just treat this silently as false rather than generating
 *  an error.
 *
 *  The USING and USE_IF macros solve this.
 *  If a Symbol is defined as
 *    #define Symbol USE_IF(condition)
 *  where condition evaluates to true (1) or false (0)
 *  then it can be tested with
 *    #if USING( Symbol )
 *  This will behave the same as '#if condition' if Symbol is defined
 *  and the value of condition is 0 (false) or 1 (true), otherwise
 *  it will generate a compilation error (a divide by zero in a constant
 *  expression).
 *
 *  To define a Symbol as always true or false, you can either use
 *    USE_IF(1) and USE_IF(0), or you can use the equivalent
 *    IN_USE and NOT_IN_USE
 *
 *  NOTE: You must not mix conventions...
 *   A Symbol defined as 0 or 1 will not work with USING( Symbol )
 *   (it will generate a compilation error)
 *   and a Symbol defined with USE_IF will always appear to be true
 *   if used directly as '#if Symbol' or '#ifdef Symbol'.
 *
 *   Unfortunately, no naming conventions currently have been adopted to
 *   distinguish between #ifdef symbols, #if symbols, and #if USING() symbols
 *   (for example symbols named IS_xxx are not always USING symbols)
 *   so currently you must be very careful to determine which kind
 *   of symbol you are working with!
 *   (At least if you use USING on a symbol not intended for it,
 *   you'll get a compilation error.)
 */
 /************************************************************************/
#pragma once

/************************************************************************/


#if !defined(USING)
#define IN_USE &&
#define NOT_IN_USE &&!
#define USE_IF(x) &&((x)?1:0)&&
#define USING(x) (1 x 1)
#endif // #if !defined(USING)

