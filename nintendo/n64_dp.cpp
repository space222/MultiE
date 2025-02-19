#include <bit>
#include "n64.h"

#define START_PENDING BIT(10)
#define END_PENDING BIT(9)
#define DMA_BUSY BIT(8)
#define CBUF_READY BIT(7)
#define PIPE_BUSY (BIT(5)|BIT(3))
#define XBUS BIT(0)

u32 n64::dp_read(u32 addr)
{
	//printf("$%X:$%X: dp_read <$%X\n", u32(cpu.pc), RSP.pc, addr);
	addr = (addr&0x1F)>>2;
	switch(addr)
	{
	case 0: return DP_START;
	case 1: return DP_END;
	case 2: return DP_CURRENT;
	
	case 3: return DP_STATUS;
	case 4:
	case 5:
	case 6:
	case 7: return 0;
	default:
		printf("DP: shouldn't happen\n");
		exit(1);
	}
	return 0;
}

void n64::dp_write(u32 addr, u32 v)
{
	//printf("dp_write $%X = $%X\n", addr, v);
	addr = (addr&0x1f)>>2;
	if( addr == 0 )
	{
		//if( !(DP_STATUS & START_PENDING) )
		{
			DP_START = v & 0x7ffff8;
		}
		DP_STATUS |= START_PENDING;
		return;
	}
	if( addr == 1 )
	{
		DP_END = v & 0x7ffff8;
		if( DP_STATUS & START_PENDING )
		{
			DP_CURRENT = DP_START;
			DP_STATUS &= ~START_PENDING;
		}
		dp_send();
		return;
	}
		
	if( addr == 3 )
	{
		u32 xbus = v&3;
		if( xbus == 1 )
		{
			DP_STATUS &= ~XBUS;
		} else if( xbus == 2 ) {
			DP_STATUS |= XBUS;
		}
		if( v & BIT(5) )
		{
			printf("Did a flush!\n");
			//exit(1);
			DP_STATUS &= ~START_PENDING;
		}
		if( v & BIT(3) )
		{
			printf("dp: froze!\n");
			//exit(1);
		}
		//if( v & BIT(7) ) DP_STATUS &= ~PIPE_BUSY;
		//if( v & 0xfc ) printf("DP_STATUS = $%X\n", v);
		return;
	}
}

void n64::dp_send()
{
	DP_STATUS |= PIPE_BUSY;
	
	//DP_CURRENT &= 0x7ffff8;
	//DP_END &= 0x7ffff8;
	
	while( DP_CURRENT != DP_END )
	{
		if( DP_STATUS & XBUS )
		{
			RDP.recv(__builtin_bswap64(*(u64*)&DMEM[DP_CURRENT&0xff8]));
		} else {
			RDP.recv(__builtin_bswap64(*(u64*)&mem[DP_CURRENT&0x7ffff8]));
		}
		DP_CURRENT += 8;
	}
	
	//DP_START = DP_CURRENT = DP_END;
	
	DP_STATUS |= CBUF_READY;
}



