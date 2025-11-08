#pragma once
#include <cstdio>

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



