#pragma once

//modified version of retro_endianness.h with INLINE and unneeded functions removed

#include <stdint.h>

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

#if defined(EMU_BIG_ENDIAN)
#define is_little_endian() (0)
#define swap_if_little64(val) (val)
#define swap_if_little32(val) (val)
#define swap_if_little16(val) (val)

#else
#define is_little_endian() (1)
#define swap_if_little64(val) (SWAP64(val))
#define swap_if_little32(val) (SWAP32(val))
#define swap_if_little16(val) (SWAP16(val))
#endif

static inline void swap16_buffer_if_little(uint16_t* buffer, uint64_t count){
#if !defined(EMU_BIG_ENDIAN)
   for(uint64_t index = 0; index < count; index++)
      buffer[index] = SWAP16(buffer[index]);
#endif
}
