#ifndef STRING_H
#define STRING_H

#include "common.h"

extern void memcpy(_u8 * dest, const _u8 * src, _u32 len);
extern void memset(void *dest, _u8 val, _u32 len);
extern int memcmp(const void *v1, const void *v2, size_t n);
extern void *memmove(void *dst, const void *src, size_t n);

extern void bzero(void *dest, _u32 len);
extern int strcmp(const char *str1, const char *str2);
extern char *strcpy(char *dest, const char *src);
extern char *strcat(char *dest, const char *src);
extern int strlen(const char *src);

#endif
