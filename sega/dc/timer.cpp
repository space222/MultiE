#include <print>
#include "dreamcast.h"
// part of SH4 internal registers

static u64 tmrdiv[] = { 4*4, 16*4, 64*4, 256*4, 1024*4, 1024*4, 1024*4, 1024*4 };

void dreamcast::timer_write(u32 addr, u64 v, u32 sz)
{
	switch( addr )
	{
	case 0xFFD80000: intern.TOCR = v&1; return; // Does this affect timer function?
	case 0xFFD80004: // TSTR
			if( (intern.TSTR&1) != (v&1) )
			{
				if( v & 1 )
				{
					sched.add_event(stamp + (intern.TCNT0*tmrdiv[intern.TCR0&7]), TMR0_UNDERFLOW);
				} else {
					sched.filter_out_event(TMR0_UNDERFLOW);
					catch_up_timer(0);
				}
			}
			if( (intern.TSTR&2) != (v&2) )
			{
				if( v & 2 )
				{
					sched.add_event(stamp + (intern.TCNT1*tmrdiv[intern.TCR1&7]), TMR1_UNDERFLOW);
				} else {
					sched.filter_out_event(TMR1_UNDERFLOW);
					catch_up_timer(1);
				}
			}
			if( (intern.TSTR&4) != (v&4) )
			{
				if( v & 4 )
				{
					sched.add_event(stamp + (intern.TCNT2*tmrdiv[intern.TCR2&7]), TMR2_UNDERFLOW);
				} else {
					sched.filter_out_event(TMR2_UNDERFLOW);
					catch_up_timer(2);
				}
			}
			intern.TSTR = v&7;
			return;
	
	case 0xFFD80008: intern.TCOR0 = v; return;
	case 0xFFD8000C: 
		intern.TCNT0 = v;
		sched.filter_out_event(TMR0_UNDERFLOW);
		if( intern.TSTR & 1 ) sched.add_event(stamp + (intern.TCNT0*tmrdiv[intern.TCR0&7]), TMR0_UNDERFLOW);
		return;
	case 0xFFD80010: 
		intern.TCR0 = v;
		sched.filter_out_event(TMR0_UNDERFLOW);
		if( intern.TSTR & 1 ) sched.add_event(stamp + (intern.TCNT0*tmrdiv[intern.TCR0&7]), TMR0_UNDERFLOW);
		return;
	
	case 0xFFD80014: intern.TCOR1 = v; return;
	case 0xFFD80018:
		intern.TCNT1 = v;
		sched.filter_out_event(TMR1_UNDERFLOW);
		if( intern.TSTR & 2 ) sched.add_event(stamp + (intern.TCNT1*tmrdiv[intern.TCR1&7]), TMR1_UNDERFLOW);
		return;
	case 0xFFD8001C:
		intern.TCR1 = v;
		sched.filter_out_event(TMR1_UNDERFLOW);
		if( intern.TSTR & 2 ) sched.add_event(stamp + (intern.TCNT1*tmrdiv[intern.TCR1&7]), TMR1_UNDERFLOW);
		return;
	
	
	case 0xFFD80020: intern.TCOR2 = v; return;
	case 0xFFD80024:
		intern.TCNT2 = v;
		sched.filter_out_event(TMR2_UNDERFLOW);
		if( intern.TSTR & 4 ) sched.add_event(stamp + (intern.TCNT2*tmrdiv[intern.TCR2&7]), TMR2_UNDERFLOW);
		return;
	case 0xFFD80028:
		intern.TCR2 = v;
		sched.filter_out_event(TMR2_UNDERFLOW);
		if( intern.TSTR & 4 ) sched.add_event(stamp + (intern.TCNT2*tmrdiv[intern.TCR2&7]), TMR2_UNDERFLOW);
		return;
	
	case 0xFFD8002C: intern.TCPR2 = v; return;// input capture unused on dreamcast?
	}
	std::println("timer write{} ${:X} = ${:X}", sz, addr, v);
	exit(1);
}

u64 dreamcast::timer_read(u32 addr, u32 sz)
{
	switch( addr )
	{
	case 0xFFD80000: return intern.TOCR;
	case 0xFFD80004: return intern.TSTR;
	case 0xFFD80008: return intern.TCOR0;
	case 0xFFD8000C: catch_up_timer(0); return intern.TCNT0;
	case 0xFFD80010: return intern.TCR0;
	
	case 0xFFD80014: return intern.TCOR1;
	case 0xFFD80018: catch_up_timer(1); return intern.TCNT1;
	case 0xFFD8001C: return intern.TCR1;
	
	case 0xFFD80020: return intern.TCOR2;
	case 0xFFD80024: catch_up_timer(2); return intern.TCNT2;
	case 0xFFD80028: return intern.TCR2;
	
	case 0xFFD8002C: return intern.TCPR2; // input capture unused on dreamcast?
	}
	std::println("timer read{} ${:X}", sz, addr);
	exit(1);
	return 0;
}

void dreamcast::catch_up_timer(u32 index)
{
	if( index == 0 && (intern.TSTR & 1) )
	{
		u64 cc = (stamp - intern.last_tmr0_read) / tmrdiv[intern.TCR0&7];
		if( cc )
		{
			intern.last_tmr0_read = stamp;
			intern.TCNT0 -= cc;
		}
	}
	else if( index == 1 && (intern.TSTR & 2) )
	{
		u64 cc = (stamp - intern.last_tmr1_read) / tmrdiv[intern.TCR1&7];
		if( cc )
		{
			intern.last_tmr1_read = stamp;
			intern.TCNT1 -= cc;
		}
	}	
	else if( index == 2 && (intern.TSTR & 4) )
	{
		u64 cc = (stamp - intern.last_tmr2_read) / tmrdiv[intern.TCR2&7];
		if( cc )
		{
			intern.last_tmr2_read = stamp;
			intern.TCNT2 -= cc;
		}
	}	
}

void dreamcast::timer_underflow(u32 index)
{



}

