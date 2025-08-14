#include <print>
#include <cstdio>
#include "gba.h"
static u16 snd88 = 0;


u32 gba::read_snd_io(u32 addr)
{
	if( addr == 0x04000088 ) { return snd88; }
	
	if( addr == 0x04000084 ) { return snd_master_en?0x80:0; }
	
	if( addr == 0x04000082 ) { return dma_sound_mix; }
	
	if( addr >= 0x04000060 && addr <= 0x04000086 )
	{
		return sndregs[(addr-0x04000060)>>1];
	}
	
	return 0;
}

void gba::write_snd_io(u32 addr, u32 v)
{
	if( addr == 0x04000088 ) { snd88 = v; return; }

	if( addr == 0x040000A0 || addr == 0x040000A2 )
	{
		snd_fifo_a.push_front(v);
		snd_fifo_a.push_front(v>>8);
		return;
	}
	if( addr == 0x040000A4 || addr == 0x040000A6 )
	{
		snd_fifo_b.push_front(v);
		snd_fifo_b.push_front(v>>8);
		return;
	}
	
	if( addr == 0x04000084 ) { snd_master_en = v&0x80; return; }
	if( addr == 0x04000082 ) 
	{
		if( v & BIT(11) ) snd_fifo_a.clear();
		if( v & BIT(15) ) snd_fifo_b.clear();
		dma_sound_mix = v & ~(BIT(11)|BIT(15));
		return; 
	}
	
	if( addr >= 0x04000060 && addr <= 0x04000086 )
	{
		sndregs[(addr-0x04000060)>>1] = v;
		return;
	}
}

void gba::snd_a_timer()
{
	if( !snd_fifo_a.empty() )
	{
		pcmA = snd_fifo_a.back()/128.f;
		snd_fifo_a.pop_back();
	}
	
	if( snd_fifo_a.size() < 16 && (dmaregs[0xB] & 0xb000)==0xb000 )
	{
		u16 b = dmaregs[0xB];
		u16 a = dmaregs[0xA];
		dmaregs[0xB] |= BIT(10)|BIT(9)|BIT(6);
		dmaregs[0xB] &= ~BIT(5);
		dmaregs[0xA] = 4;
		exec_dma(1);
		dmaregs[0xB] = b;
		dmaregs[0xA] = a;
	}
	return;
}

void gba::snd_b_timer()
{
	if( !snd_fifo_b.empty() )
	{
		pcmB = snd_fifo_b.back()/128.f; 
		snd_fifo_b.pop_back();
	}
	
	if( snd_fifo_b.size() < 16 && (dmaregs[0x11] & 0xb000)==0xb000 )
	{
		u16 b = dmaregs[0x11];
		u16 l = dmaregs[0x10];
		dmaregs[0x11] |= BIT(10)|BIT(9)|BIT(6);
		dmaregs[0x11] &= ~BIT(5);
		dmaregs[0x10] = 4;
		exec_dma(2);
		dmaregs[0x11] = b;
		dmaregs[0x10] = l;	
	}
	return;
}


