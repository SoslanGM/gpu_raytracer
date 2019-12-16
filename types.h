#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// first iteration - from standard to bit-descriptive
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint32   bool32;
typedef float    real32;
typedef double   real64;
// ---

// second iteration - bit-wise shortened
#if 1
typedef int8   s8;
typedef int16  s16;
typedef int32  s32;
typedef int64  s64;

typedef uint8  u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef bool32 b32;
typedef real32 r32;
typedef real64 r64;
#endif
// ---


// third iteration - shortened byte-wise
#if 0
typedef char   c1;

typedef int8   s1;
typedef int16  s2;
typedef int32  s4;
typedef int64  s8;

typedef uint8  u1;
typedef uint16 u2;
typedef uint32 u4;
typedef uint64 u8;

typedef bool32 b1;
typedef real32 r4;
typedef real64 r8;
#endif
// ---



#define global_variable static
#define local_persist static

#endif