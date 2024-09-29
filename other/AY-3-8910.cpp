#include "AY-3-8910.h"

void AY_3_8910::clock()
{
	out = 0;
	
	if( !(regs[7] & 1) && (regs[7]&8) )
	{
		cnt[0]+=1;
		if( cnt[0] >= (((regs[1]&0xf)<<8)|regs[0]) )
		{
			cnt[0] = 0;
			p[0] ^= 1;
		}
		if( p[0] & 1 ) out += regs[8]&0xf;
	}
	if( !(regs[7] & 2 ) && (regs[7]&0x10)  )
	{
		cnt[1]+=1;
		if( cnt[1] >= (((regs[3]&0xf)<<8)|regs[2]) )
		{
			cnt[1] = 0;
			p[1] ^= 1;
		}
		if( p[1] & 1 ) out += regs[9]&0xf;
	}
	if( !(regs[7] & 4 ) && (regs[7]&0x20) )
	{
		cnt[2]+=1;
		if( cnt[2] >= (((regs[5]&0xf)<<8)|regs[4]) )
		{
			cnt[2] = 0;
			p[2] ^= 1;
		}
		if( p[2] & 1 ) out += regs[10]&0xf;
	}
	return;
}

u8 AY_3_8910::cycles(u64 cc)
{
	stamp += cc;
	while( stamp >= 16 )
	{
		stamp -= 16;
		clock();
	}
	return out;
}


