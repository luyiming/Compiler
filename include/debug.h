#ifndef __DEBUG_H__
#define __DEBUG_H__ 1
/* uncomment next line for debug mode */
// #define YYDEBUG 1

// #define LEXDEBUG 1

// #define PRINT_AST 1

// #define ERROR_AST 1

#include <stdio.h>
#include <assert.h>
#define panic(x) printf(x "\n");assert(0)
#define unimplemented() panic("Need to implement")

#endif
