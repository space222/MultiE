#include <cstdio>
#include <cstdlib>
#include "SN79489.h"

void SN79489::run()
{
	total = 0;
	for(int i = 0; i < 3; ++i)
	{
		cnt[i]+=1;
		if( tone[i] && cnt[i] >= tone[i] )
		{
			cnt[i] = 0;
			p[i] ^= 1;
		}
		if( (p[i] & 1) || (tone[i]==1) ) total += vol[i];
	}
	
	cnt[3]+=1;
	u16 upper = 0;
	switch( tone[3] & 3 )
	{
	case 0: upper = 0x10; break;
	case 1: upper = 0x20; break;
	case 2: upper = 0x40; break;
	case 3: upper = tone[2]; break;	
	}
	if( cnt[3] >= upper )
	{
		cnt[3] = 0;
		p[3] ^= 1;
		if( p[3] & 1 )
		{
			if( tone[3] & BIT(2) )
			{
				u16 bit0 = lfsr&1;
				u16 bit3 = (lfsr>>3)&1;
				lfsr >>= 1;
				lfsr |= (bit0^bit3)<<15;
			} else {
				lfsr = (lfsr<<15)|(lfsr>>1);
			}			
		}
	}
	if( lfsr & 1 ) total += vol[3];	
}

u8 SN79489::clock(u64 cycles)
{
	stamp += cycles;
	while( stamp >= 16 )
	{
		stamp -= 16;
		run();
	}
	return total;
}

void SN79489::out(u8 data)
{
	if( data & BIT(7) )
	{
		chan = (data>>5)&3;
		reg = (data & 0x10);
		if( reg )
		{
			vol[chan] = (data&0xf)^0xf;
		} else {
			tone[chan] &= ~0xf;
			tone[chan] |= data&0xf;
		}
		return;	
	}

	if( reg )
	{
		vol[chan] = (data&0xf)^0xf;
	} else {
		tone[chan] &= ~0x3F0;
		tone[chan] |= (data&0x3f)<<4;
	}
}

void SN79489::reset()
{
	lfsr = 0x8000;
	total = stamp = 0;
	p[0] = p[1] = p[2] = p[3] = 0;
	cnt[0] = cnt[1] = cnt[2] = cnt[3] = 0;
	tone[0] = tone[1] = tone[2] = tone[3] = 0;
}

SN79489::SN79489()
{
	reset();
}

