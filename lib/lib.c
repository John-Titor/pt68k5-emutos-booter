#include "lib.h"

static inline char toupper(char c)
{
    return ((c >= 'a') && (c <= 'z')) ? c + ('A' - 'a') : c;
}

int
strcmp(const char *s1, const char *s2)
{
    for (;;) {
        char c1 = *s1++;
        char c2 = *s2++;

        if (c1 < c2) {
            return -1;
        }

        if (c1 > c2) {
            return 1;
        }

        if (c1 == 0) {
            return 0;
        }
    }
}

char *
strchr(const char *s, int i)
{
    for (; *s; ++s) {
        if (*s == i) {
            return (char *)s;
        }
    }
    return NULL;
}


int
strncmp(const char *s1, const char *s2, size_t n)
{
    while (n--) {
        char c1 = *s1++;
        char c2 = *s2++;

        if (c1 < c2) {
            return -1;
        }

        if (c1 > c2) {
            return 1;
        }

        if (c1 == 0) {
            return 0;
        }
    }

    return 0;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    while (n--) {
        char c1 = toupper(*s1++);
        char c2 = toupper(*s2++);

        c1 = toupper(c1);
        c2 = toupper(c2);

        if (c1 < c2) {
            return -1;
        }

        if (c1 > c2) {
            return 1;
        }

        if (c1 == 0) {
            return 0;
        }
    }

    return 0;
}

int
memcmp(const void *s1, const void *s2, size_t n)
{
    while (n--) {
        char c1 = *(const char *)s1++;
        char c2 = *(const char *)s2++;

        if (c1 < c2) {
            return -1;
        }

        if (c1 > c2) {
            return 1;
        }
    }

    return 0;
}

void
puts(const char *str)
{
    if (str) {
        while (*str != '\0') {
            putc(*str++);
        }
    } else {
        puts("(null)");
    }
    putc('\n');
}

static const char *hextab = "0123456789abcdef";

static void emits(void (*emit)(int c), const char *s)
{
    while (*s) {
        emit(*s++);
    }
}

static void
emitx(void (*emit)(int c), uint32_t value, size_t len)
{
    uint32_t shifts = len * 2;
    char buf[shifts + 1];
    char *p = buf + shifts;

    *p = '\0';

    do {
        uint32_t nibble = value & 0xf;
        value >>= 4;
        *--p = hextab[nibble];
    } while (p > buf);

    emits(emit, p);
}

static void
putx(uint32_t value, size_t len)
{
    emitx(putc, value, len);
}

static void
emitd(void (*emit)(int c), uint32_t value)
{
    if (value == 0) {
        putc('0');
        return;
    }

    char buf[11];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';

    while (value > 0) {
        uint32_t digit = value % 10;
        value /= 10;
        *--p = hextab[digit];
    }

    emits(emit, p);
}

// static void
// putd(uint32_t value)
// {
//     emitd(putc, value);
// }

#define WSELECT(_s, _l, _w, _b) ((_s) == 'l') ? (_l) : ((_s) == 'w') ? (_w) : (_b)

size_t
hexdump(uintptr_t addr, uintptr_t address, size_t length, char width)
{
    uint32_t index;
    uint32_t incr = WSELECT(width, 4, 2, 1);

    length &= ~(incr - 1);

    for (index = 0; index < length; index += 16) {
        putx(address + index, 8);
        putc(':');

        for (uint32_t col = 0; col < 16; col += incr) {
            putc(' ');

            if ((index + col) >= length) {
                uint32_t count = WSELECT(width, 8, 4, 2);

                while (count--) {
                    putc(' ');
                }
            } else {
                uintptr_t p = (addr + index + col);
                uint32_t val = WSELECT(width, *(uint32_t *)p, *(uint16_t *)p, *(uint8_t *)p);
                putx(val, incr);
            }
        }

        puts("  ");

        for (uint32_t col = 0; col < 16; col++) {
            if ((index + col) < length) {
                uintptr_t p = (addr + index + col);
                uint32_t c = *(char *)p;

                if ((c >= ' ') && (c < 127)) {
                    putc(c);
                } else {
                    putc('.');
                }
            } else {
                putc(' ');
            }
        }

        putc('\n');
    }

    return length;
}

/**
 * @brief      printf-style output formatter
 *
 * Supports:
 *  %c      character (char)
 *  %d      signed decimal integer (int)
 *  %u      uint32_t decimal integer (uint32_t int)
 *  %p      pointer (32-bit hex) (const void *)
 *  %b      hex byte (uint8_t)
 *  %w      hex word (uint16_t)
 *  %l      hex long (uint32_t)
 *  %s      string (const char *)
 *
 * @param[in]  format     format string
 * @param[in]  <unnamed>  format string arguments
 */
void
_fmt(void (*emit)(int c), const char *format, va_list ap)
{
    char c;
    bool dofmt = false;

    while ((c = *format++) != 0) {
        if (!dofmt) {
            if (c == '%') {
                dofmt = true;
            } else {
                emit(c);
            }

            continue;
        }

        dofmt = false;

        switch (c) {
        case 'c': {
                char c = va_arg(ap, int);
                emit(c);
                break;
            }

        case 'd': {
                int v = va_arg(ap, int);

                if (v < 0) {
                    emit('-');
                    v = -v;
                }

                emitd(emit, v);
                break;
            }

        case 'u': {
                uint32_t v = va_arg(ap, uint32_t);
                emitd(emit, v);
                break;
            }

        case 'p': {
                void *v = va_arg(ap, void *);
                emits(emit, "0x");
                emitx(emit, (uintptr_t)v, sizeof(v));
                break;
            }

        case 'b': {
                uint8_t v = va_arg(ap, uint32_t);
                emitx(emit, v, sizeof(v));
                break;
            }

        case 'w': {
                uint16_t v = va_arg(ap, uint32_t);
                emitx(emit, v, sizeof(v));
                break;
            }

        case 'l': {
                uint32_t v = va_arg(ap, uint32_t);
                emitx(emit, v, sizeof(v));
                break;
            }

        case 's': {
                const char *v = va_arg(ap, const char *);
                emits(emit, v);
                break;
            }

        case '%': {
            emit('%');
            break;
        }

        default:
            emit('%');
            emit(c);
            break;
        }
    }
}

void
vfmt(const char *format, va_list ap)
{
    _fmt(putc, format, ap);
}

void
fmt(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    _fmt(putc, format, ap);
    va_end(ap);
}
