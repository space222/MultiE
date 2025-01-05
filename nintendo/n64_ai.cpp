#include "n64.h"

#define VI_CLOCK 48726144
#define CPU_HZ 93750000

u32 n64::ai_read(u32 addr)
{
	u32 r = (addr&0x1F)>>2;
	if( r == 3 )
	{  // AI_STATUS
		u32 full = ai_buf[1].valid ? (BIT(31)|1) : 0;
		u32 busy = ai_buf[0].valid ? BIT(30) : 0;
		u32 enabled = ai_dma_enabled ? BIT(25) : 0;	
		return full|busy|enabled|BIT(24)|BIT(20);
	}
	return ai_buf[0].length & ~7; // everything but status is write-only, with reads returning length
}

void n64::ai_write(u32 addr, u32 v)
{
	u32 r = (addr&0x1F)>>2;
	if( r > 5 ) return;
	
	if( r == 0 ) 
	{	// AI_DRAM_ADDR
		if( ai_buf[0].valid ) 
		{
			ai_buf[1].ramaddr = v&0x7ffff8;
		} else {
			ai_buf[0].ramaddr = v&0x7ffff8;
		}
		return;
	}
	
	if( r == 1 )
	{	// AI_LENGTH
		if( ai_buf[0].valid )
		{
			ai_buf[1].length = v&0x3fff8;
			ai_buf[1].valid = true;
		} else {
			ai_buf[0].length = v&0x3fff8;
			ai_buf[0].valid = true;
			raise_mi_bit(MI_INTR_AI_BIT);
		}
		return;
	}
	
	if( r == 2 )
	{	// AI_CTRL
		ai_dma_enabled = v&1;
		return;
	}
	
	if( r == 3 )
	{	// AI_STATUS: writes clear the irq
		clear_mi_bit(MI_INTR_AI_BIT);
		return;
	}
	
	if( r == 4 )
	{  // AI_DACRATE
		if( v == 0 )
		{	// let's not allow a div-by-zero
			v = 1100;
		}
		float DAChz = VI_CLOCK / float(v&0x3fff);
		ai_cycles_per_sample = CPU_HZ / DAChz;
		//if( ai_cycles_per_sample < 800 ) ai_cycles_per_sample = 8000;
		printf("N64: AI config %fHz, %li cycles per sample\n", DAChz, ai_cycles_per_sample);
		return;
	}
}

