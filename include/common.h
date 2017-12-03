#ifndef __COMMON_H__
#define __COMMON_H__ 1

extern int cnt_error;
#define reportError(type, lineno, format, ...) \
printf("Error type %s at Line %d: " format "\n", type, lineno, ## __VA_ARGS__); \
cnt_error ++

typedef int bool;
enum { false, true };

#endif