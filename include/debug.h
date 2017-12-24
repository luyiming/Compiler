#ifndef __DEBUG_H__
#define __DEBUG_H__ 1
/* uncomment next line for debug mode */
// #define YYDEBUG

// #define LEXDEBUG

// #define PRINT_AST

// #define ERROR_AST

#include <stdio.h>
#include <assert.h>
#define panic(x) printf(x "\n");assert(0)
#define unimplemented() panic("Need to implement")

#endif
