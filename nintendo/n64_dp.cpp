#include "n64.h"

#define START_PENDING BIT(10)

u32 n64::dp_read(u32 addr)
{
	addr = (addr&0x1F)>>2;
	if( addr < 4 ) return dp_regs[addr];
	return 0;
}

void n64::dp_write(u32 addr, u32 v)
{
	addr = (addr&0x1f)>>2;
	if( addr == 0 )
	{
		DP_STATUS |= START_PENDING;
		DP_START = v & 0x7ffff8;
		DP_CURRENT = DP_START;
		return;
	}
	if( addr == 1 )
	{
		DP_STATUS &= ~START_PENDING;
		DP_END = v & 0x7ffff8;
		if( DP_CURRENT >= DP_END ) return;
		u32 num_dwords = (DP_END - DP_CURRENT)>>3;
		if( DP_STATUS & 1 )
		{
			RDP.run_commands((u64*)((DP_CURRENT&0xff8) + DMEM), num_dwords);
		} else {
			RDP.run_commands((u64*)(DP_CURRENT + mem.data()), num_dwords);
		}
		DP_CURRENT = DP_END;
		return;
	}
	
	if( addr == 3 )
	{
		u32 xbus = v&3;
		if( xbus == 1 )
		{
			DP_STATUS &= ~1;
		} else if( xbus == 2 ) {
			DP_STATUS |= 1;
		}
		return;
	}
}




