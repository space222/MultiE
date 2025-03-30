/*
#include <cstdio>
#include <cstring>
#include <cstdlib>
//#include <SDL.h>
#include <SFML/Graphics.hpp>
#include "types.h"

extern u16 IF;

void mem_write8(u32, u8);
void mem_write32(u32, u32);
u32 mem_read32(u32);
u16 mem_read16(u32);
void mem_write16(u32, u16);

u16 timer_regs[8] = {0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff};
u32 timer_cnt[4] = {0};
u32 tcycles[4] = {0};
const u32 prescaler[4] = { 1, 64, 256, 1024 };
bool overflowed[4] = {0};

void timer_cycles(u32 cycles)
{
	overflowed[0] = overflowed[1] = overflowed[2] = overflowed[3] = false;
	for(int T = 0; T < 4; ++T)
	{
		if( !(timer_regs[(T<<1) + 1]&0x80) ) continue;  // timer disabled
		if( T && (timer_regs[(T<<1) + 1]&4) ) 
		{ // timer counts on prev timer overflow, but not timer 0 which has no prev timer
			if( ! overflowed[T-1] ) continue;
			timer_cnt[T]++;	
		} else {
			tcycles[T] += cycles;
			const u32 prescale = prescaler[timer_regs[(T<<1)+1]&3];
			if( tcycles[T] >= prescale )
			{
				timer_cnt[T] += tcycles[T] / prescale;
				tcycles[T] = tcycles[T] % prescale;
			}
		}
		
		if( ! (timer_cnt[T] >> 16) ) continue;
		// overflow gets here
		overflowed[T] = true;
		timer_cnt[T] = timer_regs[T<<1];
		//tcycles[T] = 0;
		if( timer_regs[(T<<1)+1] & 0x40 ) IF |= 1u << (3+T);
	}
	return;
}

void timers_write16(u32 addr, u16 val)
{
	if( addr > 0x0400010F ) return;
	addr >>= 1;
	addr &= 7;
	timer_regs[addr] = val;
	if( (addr & 1) && (val&0x80) )
	{
		timer_cnt[addr>>1] = timer_regs[addr&~1];
		tcycles[addr>>1] = 0;
	}
	return;
}

u16 timers_read16(u32 addr)
{
	if( addr > 0x0400010F ) return 0;
	addr >>= 1;
	addr &= 7;
	if( addr & 1 ) return timer_regs[addr];
	return 0xffff; //timer_cnt[addr>>1];
}

*/
