#include <print>
#include "dreamcast.h"

u64 dreamcast::gdread(u32 addr, u32 sz)
{
	if( addr == GD_ALT_STAT )
	{
		if( gd.curcmd == 0xa0 )
		{
			gd.stat &= ~BIT(7);
			gd.stat ^= BIT(3);	
		}
		return gd.stat;
	}
	std::println("GD Read{} <${:X}", sz, addr);
	if( addr == GD_STAT_CMD )
	{
		holly.sb_istext &= ~1;
		if( gd.curcmd == 0xa0 )
		{
			gd.stat &= ~BIT(7);
			gd.stat ^= BIT(3);
		}
		return gd.stat;
	}
	return 0;
}

int writecnt = 0;

void dreamcast::gdwrite(u32 addr, u64 v, u32 sz)
{
	if( addr == GD_ALT_STAT ) { gd.dev_ctrl = v&15; return; }
	if( addr == GD_STAT_CMD )
	{
		std::println("GD Cmd = ${:X}", v);
		if( v == 0xEF )
		{
			holly.sb_istext |= 1;
			return;
		}
		if( v == 0xA0 )
		{
			gd.curcmd = 0xa0;
			gd.stat |= 0x80;
		}		
		return;
	}
	if( addr == 0x5F7080 )
	{
		writecnt += 1;
		if( writecnt == 6 )
		{
			writecnt = 0;
			gd.stat = BIT(6);
			holly.sb_istext |= 1;
		}
	}
	std::println("GD Write{} ${:X} = ${:X}", sz, addr, v);
}


