#include "gba.h"

static u16 tmr[4] = {0};

void gba::write_tmr_io(u32 addr, u32 v)
{








	return;
}

u32 gba::read_tmr_io(u32 addr)
{
	if( addr == 0x04000100 ) return tmr[0]+=4;
	if( addr == 0x04000104 ) return tmr[1]+=4;
	if( addr == 0x04000108 ) return tmr[2]+=4;
	if( addr == 0x0400010C ) return tmr[3]+=4;

	return 0xffff;
}
