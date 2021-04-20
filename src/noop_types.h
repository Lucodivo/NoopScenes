#pragma once

#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef s32 b32;

#define local_access static
#define file_access static
#define class_access static

#define Pi32 3.14159265359f
#define PiOverTwo32 (Pi32 / 2.0f)
#define Tau32 6.28318530717958647692f
#define RadiansPerDegree (Pi32 / 180.0f)
#define U32_MAX ~0u

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#ifdef NDEBUG
#define Assert(Expression)
#else
#define Assert(Expression) if(!(Expression)) {*(u8*)0 = 0;}
#endif

#define InvalidCodePath Assert(!"InvalidCodePath");

struct Extent2D
{
  u32 width;
  u32 height;
};

// TODO: Bring this into noop_math if ever needed?
struct vec2_64 {
  f64 x;
  f64 y;
};

b32 consumabool(b32* boolPtr) {
  b32 originalVal = *boolPtr;
  *boolPtr = false;
  return(originalVal);
}
