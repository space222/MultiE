#include <print>
#include "dreamcast.h"

void dreamcast::aica_write(u32 addr, u32 v, int sz)
{
	if( addr < 0x800000 )
	{
		sized_write(sndram, addr&0x1FFfff, v, sz);
		return;
	}
	//std::print("arm-");
	snd_write(addr & 0xffff, v, sz);
}

u32 dreamcast::aica_read(u32 addr, int sz)
{
	//std::print("arm${:X}-", aica.cpu.r[15]-8);
	if( addr < 0x800000 )
	{
		u32 got = sized_read(sndram, addr&0x1FFfff, sz);
		//std::println("arm rd${:X} = ${:X}", addr, got);
		return got;
	}
	return snd_read(addr & 0xffff, sz);
}

void dreamcast::snd_write(u32 addr, u64 v, u32 sz)
{
	addr &= 0xffff;
	if( addr >= 0xffe8 ) return;
	if( addr == ARM_RESET_ADDR )
	{
		if( aica.armrst && !(v&1) ) { aica.cpu.reset(); aica.cpu.cpsr.v &= ~0x1f; aica.cpu.cpsr.v |= ARM_MODE_SYSTEM; } //todo: double check reset()
		aica.armrst = v&1;
		return;
	}
}

u32 dreamcast::snd_read(u32 addr, u32 sz)
{
	addr &= 0xffff;
	if( addr >= 0xffe8 ) return 0;
	if( addr == ARM_RESET_ADDR ) { return aica.armrst; }
	return 0xffffffffu;
}


