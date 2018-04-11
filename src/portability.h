#pragma once

#include <stdint.h>

#if defined(BIG_ENDIAN) || defined(MSB_FIRST)

//host and be64 are the same thing
#define hostToBe64(x) x
#define be64ToHost(x) x

#else

static inline uint64_t NO_DIRECT_CALL_swap64(uint64_t data){
   uint64_t swapped = 0;
   
   swapped |= data >> 56;
   swapped |= (data & 0x00FF000000000000) >> 40;
   swapped |= (data & 0x0000FF0000000000) >> 24;
   swapped |= (data & 0x000000FF00000000) >> 8;
   swapped |= (data & 0x00000000FF000000) << 8;
   swapped |= (data & 0x0000000000FF0000) << 24;
   swapped |= (data & 0x000000000000FF00) << 40;
   swapped |= data << 56;
   
   return swapped;
}

#define hostToBe64(x) NO_DIRECT_CALL_swap64(x)
#define be64ToHost(x) NO_DIRECT_CALL_swap64(x)

#endif

static inline uint64_t getUint64FromDouble(double data){
   uint64_t* toPtr = (uint64_t*)&data;
   return *toPtr;
}

//if bool is typedefed to uint8_t and a 0x100 or above value is passed as a bool it will implicit cast to false
//other systems will just be optimize this out
#define CAST_TO_BOOL(x) ((x) != 0)
