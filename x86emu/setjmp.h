#pragma once

#include <stdint.h>
typedef struct {
    intptr_t buf[5];
} jmp_buf;

#define setjmp(_b)          __builtin_setjmp(&_b.buf)
#define longjmp(_b, _x)     __builtin_longjmp(&_b.buf, 1)
