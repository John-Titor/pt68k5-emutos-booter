/*
 * String functions
 */
#pragma once
#include <stddef.h>

extern size_t strlen(const char *s);
extern char *strchr(const char *s, int i);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern int strncasecmp(const char *s1, const char *s2, size_t n);
extern void *memset(void *b, int c, size_t len);
extern void *memcpy(void *restrict dst, const void *restrict src, size_t len);
extern int memcmp(const void *s1, const void *s2, size_t n);
