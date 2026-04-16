#include <print>
#include <string>
#include "nds.h"
#include "util.h"

u32 nds::arm9_read(u32 a, int sz, ARM_CYCLE)
{
	//todo: dtcm
	if( a >= 0x02000000u && a < 0x03000000u )
	{
		return sized_read(mainram, a&(4_MB-1), sz);
	}
	if( a >= 0x06800000u && a < 0x07000000u )
	{
		return sized_read(vram, a-0x06800000u, sz);
	}
	if( a >= 0xffff0000u ) { return sized_read(arm9_bios, a&0x7fff, sz); }
	std::println("arm9 rd{} ${:X}", sz, a);
	return 0;
}

void nds::arm9_write(u32 a, u32 v, int sz, ARM_CYCLE)
{
	if( a >= 0x02000000u && a < 0x03000000u )
	{
		return sized_write(mainram, a&(4_MB-1), v, sz);
	}
	if( a >= 0x06800000u && a < 0x07000000u )
	{
		//std::println("vram ${:X} = ${:X}", a,v);
		return sized_write(vram, a-0x06800000u, v, sz);
	}

	std::println("arm9 wr{} ${:X} = ${:X}", sz, a, v);
}

u32 nds::arm9_fetch(u32 a, int sz, ARM_CYCLE)
{
	if( a >= 0x02000000u && a < 0x03000000u )
	{
		return sized_read(mainram, a&(4_MB-1), sz);
	}
	std::println("arm9 fetch{} ${:X}", sz, a);
	return 0;
}


