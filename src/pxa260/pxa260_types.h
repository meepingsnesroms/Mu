#ifndef UARM_TYPES_H
#define UARM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "../emulator.h"

typedef uint64_t UInt64;
typedef int64_t Int64;
typedef uint32_t UInt32;
typedef int32_t Int32;
typedef uint16_t UInt16;
typedef int16_t Int16;
typedef uint8_t UInt8;
typedef int8_t Int8;
typedef uint8_t Err;
typedef uint8_t Boolean;//must use uint8_t, some functions store extra info in the other bits

#define _INLINE_ inline
#define _UNUSED_

#define TYPE_CHECK ((sizeof(UInt32) == 4) && (sizeof(UInt16) == 2) && (sizeof(UInt8) == 1))

#define errNone		0x00
#define errInternal	0x01

/* runtime stuffs */
#define err_str(str) debugLog(str)
#define err_hex(num) debugLog("0x%X", num)
#define err_dec(num) debugLog("%d", num)

#define __mem_zero(mem, len) memset(mem, 0x00, len)
#define __mem_copy(dst, src, size) memcpy(dst, src, size)

#endif
