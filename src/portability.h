#ifndef PORTABILITY_H
#define PORTABILITY_H

#include <stdint.h>
#include <stdbool.h>

//threads
#if defined(EMU_MULTITHREADED)
#define PRAGMA_STRINGIFY(x) _Pragma(#x)
#define MULTITHREAD_LOOP(x) PRAGMA_STRINGIFY(omp parallel for private(x))
#define MULTITHREAD_DOUBLE_LOOP(x, y) PRAGMA_STRINGIFY(omp parallel for collapse(2) private(x, y))
#else
#define MULTITHREAD_LOOP(x)
#define MULTITHREAD_DOUBLE_LOOP(x, y)
#endif

//pipeline
#if defined(EMU_MANAGE_HOST_CPU_PIPELINE)
#define unlikely(x) __builtin_expect(!!(x), false)
#define likely(x) __builtin_expect(!!(x), true)
#define likely_equal(x, y) __builtin_expect((x), (y))
#else
#define unlikely(x) x
#define likely(x) x
#define likely_equal(x, y) x
#endif

//endian
#define SWAP_16(x) ((uint16_t)((((uint16_t)(x) & 0x00FF) << 8) | (((uint16_t)(x) & 0xFF00) >> 8)))
#define SWAP_32(x) ((uint32_t)((((uint32_t)(x) & 0x000000FF) << 24) | (((uint32_t)(x) & 0x0000FF00) <<  8) | (((uint32_t)(x) & 0x00FF0000) >>  8) | (((uint32_t)(x) & 0xFF000000) >> 24)))
#define SWAP_64(x) ((((uint64_t)(x) & UINT64_C(0x00000000000000FF)) << 56) | (((uint64_t)(x) & UINT64_C(0x000000000000FF00)) << 40) | (((uint64_t)(x) & UINT64_C(0x0000000000FF0000)) << 24) | (((uint64_t)(x) & UINT64_C(0x00000000FF000000)) << 8) | (((uint64_t)(x) & UINT64_C(0x000000FF00000000)) >> 8) | (((uint64_t)(x) & UINT64_C(0x0000FF0000000000)) >> 24) | (((uint64_t)(x) & UINT64_C(0x00FF000000000000)) >> 40) | (((uint64_t)(x) & UINT64_C(0xFF00000000000000)) >> 56))

static inline void swap16(uint8_t* buffer, uint32_t count){
   uint32_t index;

   //count specifys the number of uint16_t's that need to be swapped, the uint8_t* is because of alignment restrictions that crash on some platforms
   count *= sizeof(uint16_t);
   MULTITHREAD_LOOP(index) for(index = 0; index < count; index += 2){
      uint8_t temp = buffer[index];
      buffer[index] = buffer[index + 1];
      buffer[index + 1] = temp;
   }
}

static inline void swap32(uint8_t* buffer, uint32_t count){
   uint32_t index;

   //count specifys the number of uint16_t's that need to be swapped, the uint8_t* is because of alignment restrictions that crash on some platforms
   count *= sizeof(uint32_t);
   MULTITHREAD_LOOP(index) for(index = 0; index < count; index += 4){
      uint8_t temp = buffer[index];
      buffer[index] = buffer[index + 3];
      buffer[index + 3] = temp;
      temp = buffer[index + 1];
      buffer[index + 1] = buffer[index + 2];
      buffer[index + 2] = temp;
   }
}

#if !defined(EMU_BIG_ENDIAN)
#define swap16BufferIfLittle(buffer, count) swap16((buffer), (count))
#define swap32BufferIfLittle(buffer, count) swap32((buffer), (count))

#define swap16BufferIfBig(buffer, count)
#define swap32BufferIfBig(buffer, count)
#else
#define swap16BufferIfLittle(buffer, count)
#define swap32BufferIfLittle(buffer, count)

#define swap16BufferIfBig(buffer, count) swap16((buffer), (count))
#define swap32BufferIfBig(buffer, count) swap32((buffer), (count))
#endif


//custom operators
#define SIZEOF_BITS(value) (sizeof(value) * 8)

static inline uintmax_t fillBottomWith0s(uintmax_t value, uint8_t count){
   return value & UINTMAX_MAX << count;
}

static inline uintmax_t fillTopWith0s(uintmax_t value, uint8_t count){
   return value & UINTMAX_MAX >> count;
}

static inline uintmax_t fillBottomWith1s(uintmax_t value, uint8_t count){
   return value | (UINTMAX_MAX >> SIZEOF_BITS(uintmax_t) - count);
}

static inline uintmax_t fillTopWith1s(uintmax_t value, uint8_t count){
   return value | (UINTMAX_MAX << SIZEOF_BITS(uintmax_t) - count);
}

static inline uintmax_t leftShiftUse1s(uintmax_t value, uint8_t count){
   return fillBottomWith1s(value << count, count);
}

static inline uintmax_t rightShiftUse1s(uintmax_t value, uint8_t count){
   return fillTopWith1s(value >> count, count);
}

//range capping
#define FAST_MIN(x, y) ((x) < (y) ? (x) : (y))
#define FAST_MAX(x, y) ((x) > (y) ? (x) : (y))
#define FAST_ABS(x) ((x) < 0 ? -(x) : (x))

//float platform safety
static inline uint64_t getUint64FromDouble(double data){
   //1.32.31 fixed point
   uint64_t fixedPointDouble = UINT64_C(0x0000000000000000);

   if(data < 0.0){
      data = -data;
      fixedPointDouble |= UINT64_C(0x8000000000000000);
   }

   fixedPointDouble |= (uint64_t)data << 31;
   data -= (uint64_t)data;
   data *= (double)0x7FFFFFFF;
   fixedPointDouble |= (uint64_t)data;
   
   return fixedPointDouble;
}

static inline double getDoubleFromUint64(uint64_t data){
   //1.32.31 fixed point
   double floatingPointDouble;

   floatingPointDouble = (double)(data & 0x7FFFFFFF);
   floatingPointDouble /= (double)0x7FFFFFFF;
   floatingPointDouble += (double)(data >> 31 & 0xFFFFFFFF);
   if(data & UINT64_C(0x8000000000000000))
      floatingPointDouble = -floatingPointDouble;
   
   return floatingPointDouble;
}

//savestate platform safety
static inline uint64_t readStateValue64(uint8_t* where){
   return (uint64_t)where[0] << 56 | (uint64_t)where[1] << 48 | (uint64_t)where[2] << 40 | (uint64_t)where[3] << 32 | (uint64_t)where[4] << 24 | (uint64_t)where[5] << 16 | (uint64_t)where[6] << 8 | (uint64_t)where[7];
}

static inline void writeStateValue64(uint8_t* where, uint64_t value){
   where[0] = value >> 56;
   where[1] = value >> 48 & 0xFF;
   where[2] = value >> 40 & 0xFF;
   where[3] = value >> 32 & 0xFF;
   where[4] = value >> 24 & 0xFF;
   where[5] = value >> 16 & 0xFF;
   where[6] = value >> 8 & 0xFF;
   where[7] = value & 0xFF;
}

static inline uint32_t readStateValue32(uint8_t* where){
   return (uint32_t)where[0] << 24 | (uint32_t)where[1] << 16 | (uint32_t)where[2] << 8 | (uint32_t)where[3];
}

static inline void writeStateValue32(uint8_t* where, uint32_t value){
   where[0] = value >> 24;
   where[1] = value >> 16 & 0xFF;
   where[2] = value >> 8 & 0xFF;
   where[3] = value & 0xFF;
}

static inline uint16_t readStateValue16(uint8_t* where){
   return (uint16_t)where[0] << 8 | (uint16_t)where[1];
}

static inline void writeStateValue16(uint8_t* where, uint16_t value){
   where[0] = value >> 8;
   where[1] = value & 0xFF;
}

static inline uint8_t readStateValue8(uint8_t* where){
   return where[0];
}

static inline void writeStateValue8(uint8_t* where, uint8_t value){
   where[0] = value;
}

static inline double readStateValueDouble(uint8_t* where){
   return getDoubleFromUint64(readStateValue64(where));
}

static inline void writeStateValueDouble(uint8_t* where, double value){
   writeStateValue64(where, getUint64FromDouble(value));
}

#endif
