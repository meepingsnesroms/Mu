#pragma once

#include <stdint.h>
#include <stdbool.h>


//endian
#define SWAP16(x) ((uint16_t)(                  \
         (((uint16_t)(x) & 0x00ff) << 8)      | \
         (((uint16_t)(x) & 0xff00) >> 8)        \
          ))

static inline void swap16_buffer_if_little(uint16_t* buffer, uint64_t count){
#if !defined(EMU_BIG_ENDIAN)
   for(uint64_t index = 0; index < count; index++)
      buffer[index] = SWAP16(buffer[index]);
#endif
}


//threads
#if defined(EMU_MULTITHREADED)
#define MULTITHREAD_LOOP _Pragma("omp parallel for")
#define MULTITHREAD_DOUBLE_LOOP _Pragma("omp parallel for collapse(2)")
#else
#define MULTITHREAD_LOOP
#define MULTITHREAD_DOUBLE_LOOP
#endif


static inline const char* boolString(bool boo){
   return boo ? "true" : "false";
}

static inline uint64_t uMin(uint64_t x, uint64_t y){
   return x < y ? x : y;
}

static inline uint64_t uMax(uint64_t x, uint64_t y){
   return x > y ? x : y;
}

static inline uint64_t uClamp(uint64_t low, uint64_t value, uint64_t high){
   //x must always be less than z!
   return uMax(low, uMin(value, high));
}

static inline int64_t sMin(int64_t x, int64_t y){
   return x < y ? x : y;
}

static inline int64_t sMax(int64_t x, int64_t y){
   return x > y ? x : y;
}

static inline int64_t sClamp(int64_t low, int64_t value, int64_t high){
   //x must always be less than z!
   return sMax(low, sMin(value, high));
}

static inline double dMin(double x, double y){
   return x < y ? x : y;
}

static inline double dMax(double x, double y){
   return x > y ? x : y;
}

static inline double dClamp(double low, double value, double high){
   //x must always be less than z!
   return dMax(low, dMin(value, high));
}


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
   floatingPointDouble += (double)(data >> 31 & 0xFFFFFFFF);
   if(data & 0x8000000000000000)
      floatingPointDouble = -floatingPointDouble;
   
   return floatingPointDouble;
}

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

static inline bool readStateValueBool(uint8_t* where){
   return where[0];
}

static inline void writeStateValueBool(uint8_t* where, bool value){
   where[0] = value;
}
