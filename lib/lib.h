/*
 * Library functions
 */

#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* misc.S */
extern void __attribute__((noreturn)) jump_to_loaded_os(uint32_t entrypoint);
extern bool probe(uint32_t address);

/* monk5.S */
extern void putc(int c);
extern uint32_t trap12(uint8_t op, uint8_t arg1, uint8_t arg2, const void *addr);

/* lib.c */
extern void puts(const char *str);
extern size_t hexdump(uintptr_t addr, uintptr_t address, size_t length, char width);
extern void fmt(const char *format, ...);
extern void vfmt(const char *format, va_list ap);
