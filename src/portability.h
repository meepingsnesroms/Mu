#pragma once

#include <stdint.h>
#include <stdbool.h>


//endian
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

static inline double readStateValueDouble(uint8_t* where){
   return getDoubleFromUint64(swap_if_little64(*(uint64_t*)where));
}

static inline void writeStateValueDouble(uint8_t* where, double value){
   *(uint64_t*)where = swap_if_little64(getUint64FromDouble(value));
}

static inline uint64_t readStateValueUint64(uint8_t* where){
   return swap_if_little64(*(uint64_t*)where);
}

static inline void writeStateValueUint64(uint8_t* where, uint64_t value){
   *(uint64_t*)where = swap_if_little64(value);
}

static inline int64_t readStateValueInt64(uint8_t* where){
   return swap_if_little64(*(uint64_t*)where);
}

static inline void writeStateValueInt64(uint8_t* where, int64_t value){
   *(uint64_t*)where = swap_if_little64(value);
}

static inline uint32_t readStateValueUint32(uint8_t* where){
   return swap_if_little32(*(uint32_t*)where);
}

static inline void writeStateValueUint32(uint8_t* where, uint32_t value){
   *(uint32_t*)where = swap_if_little32(value);
}

static inline int32_t readStateValueInt32(uint8_t* where){
   return swap_if_little32(*(uint32_t*)where);
}

static inline void writeStateValueInt32(uint8_t* where, int32_t value){
   *(uint32_t*)where = swap_if_little32(value);
}

static inline uint16_t readStateValueUint16(uint8_t* where){
   return swap_if_little16(*(uint16_t*)where);
}

static inline void writeStateValueUint16(uint8_t* where, uint16_t value){
   *(uint16_t*)where = swap_if_little16(value);
}

static inline int16_t readStateValueInt16(uint8_t* where){
   return swap_if_little16(*(uint16_t*)where);
}

static inline void writeStateValueInt16(uint8_t* where, int16_t value){
   *(uint16_t*)where = swap_if_little16(value);
}

static inline uint8_t readStateValueUint8(uint8_t* where){
   return *where;
}

static inline void writeStateValueUint8(uint8_t* where, uint8_t value){
   *where = value;
}

static inline int8_t readStateValueInt8(uint8_t* where){
   return *where;
}

static inline void writeStateValueInt8(uint8_t* where, int8_t value){
   *where = value;
}

static inline bool readStateValueBool(uint8_t* where){
   return *where;
}

static inline void writeStateValueBool(uint8_t* where, bool value){
   *where = value;
}
