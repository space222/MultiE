#include <cstdio>
#include <cstdlib>
#include "psx.h"

void psx::tick_timer_zero(u32 ticks)
{
	u32 oldval = t0_val&0xffff;
	t0_val += ticks;
	if( (oldval < t0_target && t0_val >= t0_target) )
	{
		t0_mode |= (1u<<11);
		if( (t0_mode&0x10) ) 
		{
			I_STAT |= IRQ_TIM0;
			t0_mode &= ~(1u<<10);
		}
		if( t0_mode & 8 ) t0_val = 0;
	} else if( (oldval < 0xffff && t0_val >= 0xffff) ) {
		t0_mode |= (1u<<12);
		if( (t0_mode&0x20) ) I_STAT |= IRQ_TIM0;
		if( !(t0_mode & 8) ) t0_val = 0;
	}
}

void psx::tick_timer_one(u32 ticks)
{
	u32 oldval = t1_val&0xffff;
	t1_val += ticks;
	if( (oldval < t1_target && t1_val >= t1_target) )
	{
		t1_mode |= (1u<<11);
		if( (t1_mode&0x10) )
		{
			I_STAT |= IRQ_TIM1;
			t1_mode &= ~(1u<<10);
		}
		if( t1_mode & 8 ) t1_val = 0;
	} else if( (oldval < 0xffff && t1_val >= 0xffff) ) {
		t1_mode |= (1u<<12);
		if( (t1_mode&0x20) ) I_STAT |= IRQ_TIM1;
		if( !(t1_mode & 8) ) t1_val = 0;
	}
}

void psx::tick_timer_two(u32 ticks)
{
	u32 clksrc = (t2_mode>>8)&3;
	if( clksrc >= 2 )
	{
		t2_div += ticks;
		ticks = t2_div/8;
		t2_div %= 8;
		if( ticks == 0 ) return;
	}
	u32 oldval = t2_val&0xffff;
	t2_val += ticks;
	if( (oldval < t2_target && t2_val >= t2_target) )
	{
		//t2_mode |= (1u<<11);
		if( (t2_mode&0x10) ) 
		{
			I_STAT |= IRQ_TIM2;
			//t2_mode &= ~(1u<<10);
		}
		if( t2_mode & 8 ) t2_val = 0;
	} else if( (oldval < 0xffff && t2_val >= 0xffff) ) {
		//t2_mode |= (1u<<12);
		if( (t2_mode&0x20) )
		{
			I_STAT |= IRQ_TIM2;
			//t2_mode &= ~(1u<<10);
		}
		if( !(t2_mode & 8) ) t2_val = 0;
	}
}

