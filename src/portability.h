#ifndef PORTABILITY_H
#define PORTABILITY_H

#include <stdint.h>
#include <stdbool.h>

//endian
static inline void swap16BufferIfLittle(uint8_t* buffer, uint64_t count){
#if !defined(EMU_BIG_ENDIAN)
   uint64_t index;
   //count specifys the number of uint16_t's that need to be swapped, the uint8_t* is because of alignment restrictions that crash on some platforms
   count *= sizeof(uint16_t);
   for(index = 0; index < count; index += 2){
      uint8_t temp = buffer[index];
      buffer[index] = buffer[index + 1];
      buffer[index + 1] = temp;
   }
#endif
}

//threads
#if defined(EMU_MULTITHREADED)
#define PRAGMA_STRINGIFY(x) _Pragma(#x)
#define MULTITHREAD_LOOP(x) PRAGMA_STRINGIFY(omp parallel for private(x))
#define MULTITHREAD_DOUBLE_LOOP(x, y) PRAGMA_STRINGIFY(omp parallel for collapse(2) private(x, y))
#else
#define MULTITHREAD_LOOP(x)
#define MULTITHREAD_DOUBLE_LOOP(x, y)
#endif

//range capping
static inline uint8_t u8Min(uint8_t x, uint8_t y){
   return x < y ? x : y;
}

static inline uint8_t u8Max(uint8_t x, uint8_t y){
   return x > y ? x : y;
}

static inline uint8_t u8Clamp(uint8_t low, uint8_t value, uint8_t high){
   //low must always be less than high!
   return u8Max(low, u8Min(value, high));
}

static inline uint16_t u16Min(uint16_t x, uint16_t y){
   return x < y ? x : y;
}

static inline uint16_t u16Max(uint16_t x, uint16_t y){
   return x > y ? x : y;
}

static inline uint16_t u16Clamp(uint16_t low, uint16_t value, uint16_t high){
   //low must always be less than high!
   return u16Max(low, u16Min(value, high));
}

static inline uint32_t u32Min(uint32_t x, uint32_t y){
   return x < y ? x : y;
}

static inline uint32_t u32Max(uint32_t x, uint32_t y){
   return x > y ? x : y;
}

static inline uint32_t u32Clamp(uint32_t low, uint32_t value, uint32_t high){
   //low must always be less than high!
   return u32Max(low, u32Min(value, high));
}

static inline uint64_t u64Min(uint64_t x, uint64_t y){
   return x < y ? x : y;
}

static inline uint64_t u64Max(uint64_t x, uint64_t y){
   return x > y ? x : y;
}

static inline uint64_t u64Clamp(uint64_t low, uint64_t value, uint64_t high){
   //low must always be less than high!
   return u64Max(low, u64Min(value, high));
}

static inline int8_t s8Min(int8_t x, int8_t y){
   return x < y ? x : y;
}

static inline int8_t s8Max(int8_t x, int8_t y){
   return x > y ? x : y;
}

static inline int8_t s8Clamp(int8_t low, int8_t value, int8_t high){
   //low must always be less than high!
   return s8Max(low, s8Min(value, high));
}

static inline int16_t s16Min(int16_t x, int16_t y){
   return x < y ? x : y;
}

static inline int16_t s16Max(int16_t x, int16_t y){
   return x > y ? x : y;
}

static inline int16_t s16Clamp(int16_t low, int16_t value, int16_t high){
   //low must always be less than high!
   return s16Max(low, s16Min(value, high));
}

static inline int32_t s32Min(int32_t x, int32_t y){
   return x < y ? x : y;
}

static inline int32_t s32Max(int32_t x, int32_t y){
   return x > y ? x : y;
}

static inline int32_t s32Clamp(int32_t low, int32_t value, int32_t high){
   //low must always be less than high!
   return s32Max(low, s32Min(value, high));
}

static inline int64_t s64Min(int64_t x, int64_t y){
   return x < y ? x : y;
}

static inline int64_t s64Max(int64_t x, int64_t y){
   return x > y ? x : y;
}

static inline int64_t s64Clamp(int64_t low, int64_t value, int64_t high){
   //low must always be less than high!
   return s64Max(low, s64Min(value, high));
}

static inline float fMin(float x, float y){
   return x < y ? x : y;
}

static inline float fMax(float x, float y){
   return x > y ? x : y;
}

static inline float fClamp(float low, float value, float high){
   //low must always be less than high!
   return fMax(low, fMin(value, high));
}

static inline double dMin(double x, double y){
   return x < y ? x : y;
}

static inline double dMax(double x, double y){
   return x > y ? x : y;
}

static inline double dClamp(double low, double value, double high){
   //low must always be less than high!
   return dMax(low, dMin(value, high));
}

//float platform safety
static inline uint64_t getUint64FromDouble(double data){
   //1.32.31 fixed point
   uint64_t fixedPointDouble = 0x0000000000000000;

   if(data < 0.0){
      data = -data;
      fixedPointDouble |= 0x8000000000000000;
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

   floatingPointDouble = (double)(data & 0x000000007FFFFFFF);
   floatingPointDouble /= (double)0x7FFFFFFF;
   floatingPointDouble += (double)(data >> 31 & 0x00000000FFFFFFFF);
   if(data & 0x8000000000000000)
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
