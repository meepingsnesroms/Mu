#ifndef UARM_MATH_64_H
#define UARM_MATH_64_H

#include "pxa260_types.h"

static inline UInt64 u64_from_halves(UInt32 hi, UInt32 lo) { return (((UInt64)hi) << 32ULL) | ((UInt64)lo); }
static inline UInt64 u64_32_to_64(UInt32 v)                { return (UInt64)v; }
static inline UInt32 u64_64_to_32(UInt64 v)                { return (UInt32)v; }
static inline UInt32 u64_get_hi(UInt64 v)                  { return (UInt32)(v >> 32ULL); }
static inline UInt64 u64_add(UInt64 a, UInt64 b)           { return a + b; }
static inline UInt64 u64_add32(UInt64 a, UInt32 b)         { return a + (UInt64)b; }
static inline UInt64 u64_umul3232(UInt32 a, UInt32 b)      { return ((UInt64)a) * ((UInt64)b); }	//sad but true: gcc has no u32xu32->64 multiply
static inline UInt64 u64_smul3232(Int32 a, Int32 b)        { return ((Int64)a) * ((Int64)b); }	//sad but true: gcc has no s32xs32->64 multiply
static inline UInt64 u64_shr(UInt64 a, unsigned bits)      { return a >> (UInt64)bits; }
static inline UInt64 u64_shl(UInt64 a, unsigned bits)      { return a << (UInt64)bits; }
static inline UInt64 u64_xtnd32(UInt64 a)                  { if(a & 0x80000000UL) a |= (((UInt64)-1) << 32ULL); return a; }
static inline Boolean u64_isZero(UInt64 a)                 { return a == 0; }
static inline UInt64 u64_inc(UInt64 a)                     { return a + 1ULL; }
static inline UInt64 u64_and(UInt64 a, UInt64 b)           { return a & b; }
static inline UInt64 u64_zero(void)                        { return 0; }
static inline UInt64 u64_sub(UInt64 a, UInt64 b)           { return a - b; }

#endif


