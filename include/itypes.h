#pragma once
#include <cstdint>
#include <cstring>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define BIT(b) (1u<<(b))
#define BITL(b) (1ull<<(b))

#ifndef _WIN32
	#define PACKED __attribute__((packed))
#else
	#define PACKED
#endif


