#pragma once

//modified version of retro_endianness.h with INLINE and unneeded functions removed

#include <stdint.h>

#if defined(_MSC_VER) && _MSC_VER > 1200
#define SWAP16 _byteswap_ushort
#define SWAP32 _byteswap_ulong
#else
#define SWAP16(x) ((uint16_t)(                  \
         (((uint16_t)(x) & 0x00ff) << 8)      | \
         (((uint16_t)(x) & 0xff00) >> 8)        \
          ))
#define SWAP32(x) ((uint32_t)(           \
         (((uint32_t)(x) & 0x000000ff) << 24) | \
         (((uint32_t)(x) & 0x0000ff00) <<  8) | \
         (((uint32_t)(x) & 0x00ff0000) >>  8) | \
         (((uint32_t)(x) & 0xff000000) >> 24)   \
         ))
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define SWAP64(val)                                             \
	((((uint64_t)(val) & 0x00000000000000ff) << 56)      \
	 | (((uint64_t)(val) & 0x000000000000ff00) << 40)    \
	 | (((uint64_t)(val) & 0x0000000000ff0000) << 24)    \
	 | (((uint64_t)(val) & 0x00000000ff000000) << 8)     \
	 | (((uint64_t)(val) & 0x000000ff00000000) >> 8)     \
	 | (((uint64_t)(val) & 0x0000ff0000000000) >> 24)    \
	 | (((uint64_t)(val) & 0x00ff000000000000) >> 40)    \
	 | (((uint64_t)(val) & 0xff00000000000000) >> 56))
#else
#define SWAP64(val)                                             \
	((((uint64_t)(val) & 0x00000000000000ffULL) << 56)      \
	 | (((uint64_t)(val) & 0x000000000000ff00ULL) << 40)    \
	 | (((uint64_t)(val) & 0x0000000000ff0000ULL) << 24)    \
	 | (((uint64_t)(val) & 0x00000000ff000000ULL) << 8)     \
	 | (((uint64_t)(val) & 0x000000ff00000000ULL) >> 8)     \
	 | (((uint64_t)(val) & 0x0000ff0000000000ULL) >> 24)    \
	 | (((uint64_t)(val) & 0x00ff000000000000ULL) >> 40)    \
	 | (((uint64_t)(val) & 0xff00000000000000ULL) >> 56))
#endif

/**
 * is_little_endian:
 *
 * Checks if the system is little endian or big-endian.
 *
 * Returns: greater than 0 if little-endian,
 * otherwise big-endian.
 **/
#if defined(MSB_FIRST)
#define is_little_endian() (0)
#elif defined(__x86_64) || defined(__i386) || defined(_M_IX86) || defined(_M_X64) || defined(LSB_FIRST)
#define is_little_endian() (1)
#else
static inline uint8_t is_little_endian(void)
{
   union
   {
      uint16_t x;
      uint8_t y[2];
   } u;

   u.x = 1;
   return u.y[0];
}
#endif

/**
 * swap_if_little64:
 * @val        : unsigned 64-bit value
 *
 * Byteswap unsigned 64-bit value if system is little-endian.
 *
 * Returns: Byteswapped value in case system is little-endian,
 * otherwise returns same value.
 **/

#if defined(MSB_FIRST)
#define swap_if_little64(val) (val)
#elif defined(__x86_64) || defined(__i386) || defined(_M_IX86) || defined(_M_X64) || defined(LSB_FIRST)
#define swap_if_little64(val) (SWAP64(val))
#else
static inline uint64_t swap_if_little64(uint64_t val)
{
   if (is_little_endian())
      return SWAP64(val);
   return val;
}
#endif

/**
 * swap_if_little32:
 * @val        : unsigned 32-bit value
 *
 * Byteswap unsigned 32-bit value if system is little-endian.
 *
 * Returns: Byteswapped value in case system is little-endian,
 * otherwise returns same value.
 **/

#if defined(MSB_FIRST)
#define swap_if_little32(val) (val)
#elif defined(__x86_64) || defined(__i386) || defined(_M_IX86) || defined(_M_X64) || defined(LSB_FIRST)
#define swap_if_little32(val) (SWAP32(val))
#else
static inline uint32_t swap_if_little32(uint32_t val)
{
   if (is_little_endian())
      return SWAP32(val);
   return val;
}
#endif

/**
 * swap_if_little16:
 * @val        : unsigned 16-bit value
 *
 * Byteswap unsigned 16-bit value if system is little-endian.
 *
 * Returns: Byteswapped value in case system is little-endian,
 * otherwise returns same value.
 **/

#if defined(MSB_FIRST)
#define swap_if_little16(val) (val)
#elif defined(__x86_64) || defined(__i386) || defined(_M_IX86) || defined(_M_X64) || defined(LSB_FIRST)
#define swap_if_little16(val) (SWAP16(val))
#else
static inline uint16_t swap_if_little16(uint16_t val)
{
   if (is_little_endian())
      return SWAP16(val);
   return val;
}
#endif
