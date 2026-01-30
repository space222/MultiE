#pragma once
#include <vector>
#include <functional>
#include <cstdio>
#include "itypes.h"

// fsize - returns the size of the file, maintains current seek position
u32 fsize(FILE* fp);

// freadall - reads the entire file from the beginning into dst, then closes it. 
//		Returns false if fp==null, otherwise true. Allowing a composed fopen call to fail through.
//		use 'max' to prevent overrun in dst. vector dst is resized if too small.
bool freadall(u8* dst, FILE* fp, u32 max=0);
bool freadall(std::vector<u8>& dst, FILE* fp);

inline u64 sized_read(u8* data, u32 addr, u32 size)
{
	if( size == 64 ) return *(u64*)&data[addr];
	if( size == 32 ) return *(u32*)&data[addr];
	if( size == 16 ) return *(u16*)&data[addr];
	if( size == 8 ) return data[addr];
	std::printf("error in sized_read, size=%i\n", size);
	exit(1);
}

inline void sized_write(u8* data, u32 addr, u64 v, u32 size)
{
	if( size == 64 ) { *(u64*)&data[addr] = v; }
	else if( size == 32 ) { *(u32*)&data[addr] = v; }
	else if( size == 16 ) { *(u16*)&data[addr] = v; }
	else if( size == 8 ) { data[addr] = v; }
	else {
		std::printf("error in sized_write, size=%i\n", size);
		exit(1);
	}
}

inline void sized_write128(u8* data, u32 addr, __uint128_t v, u32 size)
{
	if( size < 128 )
	{
		return sized_write(data, addr, v, size);
	}
	memcpy(&data[addr], &v, 16);
}

inline __uint128_t sized_read128(u8* data, u32 addr, u32 size)
{
	if( size < 128 ) return sized_read(data, addr, size);
	__uint128_t v = 0;
	memcpy(&v, &data[addr], 16);
	return v;
}

inline u64 sized_read_be(u8* data, u32 addr, u32 size)
{
	if( size == 64 ) return __builtin_bswap64(*(u64*)&data[addr]);
	if( size == 32 ) return __builtin_bswap32(*(u32*)&data[addr]);
	if( size == 16 ) return __builtin_bswap16(*(u16*)&data[addr]);
	if( size == 8 ) return data[addr];
	std::printf("error in sized_read_be, size=%i\n", size);
	exit(1);
}

inline void sized_write_be(u8* data, u32 addr, u64 v, u32 size)
{
	if( size == 64 ) { *(u64*)&data[addr] = __builtin_bswap64(v); }
	else if( size == 32 ) { *(u32*)&data[addr] = __builtin_bswap32(u32(v)); }
	else if( size == 16 ) { *(u16*)&data[addr] = __builtin_bswap16(u16(v)); }
	else if( size == 8 ) { data[addr] = v; }
	else {
		std::printf("error in sized_write_be, size=%i\n", size);
		exit(1);
	}
}

template<typename T>
T freadb(FILE* fp)
{
	T value{};
	[[maybe_unused]] int unu = fread(&value, 1, sizeof(T), fp);
	return value;
}

template<typename T>
T freadb_offs(FILE* fp, int offset)
{
	T value{};
	fseek(fp, offset, SEEK_SET);
	[[maybe_unused]] int unu = fread(&value, 1, sizeof(T), fp);
	return value;
}

u32 loadELF(FILE* fp, std::function<void(u32 addr, u8 v)> writer);

