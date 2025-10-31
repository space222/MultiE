#include <print>
#include "dreamcast.h"

void dreamcast::aica_write(u32 addr, u32 v, int sz)
{
	if( addr < 0x800000 )
	{
		sized_write(sndram, addr&0x1FFfff, v, sz);
		return;
	}
	snd_write(addr & 0xffff, v, sz);
}

u32 dreamcast::aica_read(u32 addr, int sz)
{
	if( addr < 0x800000 ) return sized_read(sndram, addr&0x1FFfff, sz);
	return snd_read(addr & 0xffff, sz);
}

void dreamcast::snd_write(u32 addr, u64 v, u32 sz)
{
	addr &= 0xffff;
	if( addr == ARM_RESET_ADDR )
	{
		aica.armrst = v&1;
		if( aica.armrst ) { aica.cpu.reset(); } //todo: double check reset()
		return;
	}
	std::println("snd-w{} ${:X} = ${:X}", sz, addr, v);
}

u32 dreamcast::snd_read(u32 addr, u32 sz)
{
	addr &= 0xffff;
	if( addr == ARM_RESET_ADDR ) { return aica.armrst; }
	std::println("snd-r{} ${:X}", sz, addr);
	return 0;
}


