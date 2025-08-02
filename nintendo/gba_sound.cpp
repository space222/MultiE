#include <print>
#include "gba.h"
static u16 snd88 = 0;


u32 gba::read_snd_io(u32 addr)
{
	if( addr == 0x04000088 ) { return snd88; }
	
	return 0;
}

void gba::write_snd_io(u32 addr, u32 v)
{
	if( addr == 0x04000088 ) { snd88 = v; return; }


}


