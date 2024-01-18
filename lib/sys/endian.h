/*
 * Minimal endian support.
 */
#include <stdint.h>

#pragma once

#define __BIG_ENDIAN__ 1

#define __GEN_ENDIAN_ENC(bits, endian) \
static inline void __attribute__((unused)) \
endian ## bits ## enc(void *dst, uint ## bits ## _t u) \
{ \
	u = hto ## endian ## bits (u); \
	__builtin_memcpy(dst, &u, sizeof(u)); \
}

__GEN_ENDIAN_ENC(16, be)
__GEN_ENDIAN_ENC(32, be)
__GEN_ENDIAN_ENC(64, be)
__GEN_ENDIAN_ENC(16, le)
__GEN_ENDIAN_ENC(32, le)
__GEN_ENDIAN_ENC(64, le)
#undef __GEN_ENDIAN_ENC

#define __GEN_ENDIAN_DEC(bits, endian) \
static inline uint ## bits ## _t __attribute__((unused)) \
endian ## bits ## dec(const void *buf) \
{ \
	uint ## bits ## _t u; \
	__builtin_memcpy(&u, buf, sizeof(u)); \
	return endian ## bits ## toh (u); \
}

__GEN_ENDIAN_DEC(16, be)
__GEN_ENDIAN_DEC(32, be)
__GEN_ENDIAN_DEC(64, be)
__GEN_ENDIAN_DEC(16, le)
__GEN_ENDIAN_DEC(32, le)
__GEN_ENDIAN_DEC(64, le)
#undef __GEN_ENDIAN_DEC
