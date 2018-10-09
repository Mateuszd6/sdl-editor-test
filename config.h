#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include <cstdint>
#include <stdio.h>

#define SUCCEEDED(EXPR_RES) (static_cast<int>(EXPR_RES) >= 0)
#define FAILED(EXPR_RES) (static_cast<int>(EXPR_RES) < 0)
#define IS_NULL(EXPR_RES) ((EXPR_RES) == nullptr)

// My debug goodies library and its config stuff.
#define DEBUG
#define LOGGING
#define DG_LOG_LVL DG_LOG_LVL_ALL // DG_LOG_LVL_WARN
#define DG_USE_COLORS 1
#include "debug_goodies.h"

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

inline int8 operator"" _i8(unsigned long long liter) { return static_cast<int8>(liter); }
inline uint8 operator"" _u8(unsigned long long liter) { return static_cast<uint8>(liter); }
inline int16 operator"" _i16(unsigned long long liter) { return static_cast<int16>(liter); }
inline uint16 operator"" _u16(unsigned long long liter) { return static_cast<uint16>(liter); }
inline int32 operator"" _i32(unsigned long long liter) { return static_cast<int32>(liter); }
inline uint32 operator"" _u32(unsigned long long liter) { return static_cast<uint32>(liter); }
inline int64 operator"" _i64(unsigned long long liter) { return static_cast<int64>(liter); }
inline uint64 operator"" _u64(unsigned long long liter) { return static_cast<uint64>(liter); }

#endif // CONFIG_H_INCLUDED
