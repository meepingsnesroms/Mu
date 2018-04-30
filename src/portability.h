#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "endianness.h"


static inline const char* boolString(bool boo){
   return boo ? "true" : "false";
}

static inline uint64_t uMin(uint64_t x, uint64_t y){
   return x < y ? x : y;
}

static inline uint64_t uMax(uint64_t x, uint64_t y){
   return x > y ? x : y;
}

static inline int64_t sMin(int64_t x, int64_t y){
   return x < y ? x : y;
}

static inline int64_t sMax(int64_t x, int64_t y){
   return x > y ? x : y;
}


static inline uint64_t getUint64FromDouble(double data){
   //32.32 fixed point
   uint64_t fixedPointDouble = ((uint64_t)data) << 32;
   data -= (uint64_t)data;
   data *= 100000000.0;
   fixedPointDouble |= (uint64_t)data;
   
   return fixedPointDouble;
}

static inline double getDoubleFromUint64(uint64_t data){
   //32.32 fixed point
   double floatingPointDouble = (double)(data & 0x00000000FFFFFFFF);
   floatingPointDouble /= 100000000.0;
   floatingPointDouble += (double)(data >> 32);
   
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
