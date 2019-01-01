#ifndef UARM_TYPES_H
#define UARM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef uint64_t UInt64;
typedef int64_t Int64;
typedef uint32_t UInt32;
typedef int32_t Int32;
typedef uint16_t UInt16;
typedef int16_t Int16;
typedef uint8_t UInt8;
typedef int8_t Int8;
typedef uint8_t Err;
typedef bool Boolean;//may need to be uint8_t, thats its type in the original source

#define errNone		0x00
#define errInternal	0x01

/*
#define _INLINE_   	inline __attribute__ ((always_inline))
#define _UNUSED_	__attribute__((unused))
*/
#define _INLINE_ inline
#define _UNUSED_

/* runtime stuffs */
/*
void err_str(const char* str);
void err_hex(UInt32 val);
void err_dec(UInt32 val);
*/
#define err_str(x)
#define err_hex(x)
#define err_dec(x)

#define __mem_zero(mem, len) memset(mem, 0x00, len)
#define __mem_copy(d, s, sz) memcpy(d, s, sz)

#endif

