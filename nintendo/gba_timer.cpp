#include "gba.h"

static const u64 prescaler[4] = { 1, 64, 256, 1024 };

void gba::write_tmr_io(u32 addr, u32 v)
{
	//std::println("timer io write ${:X} = ${:X}", addr, v);
	bool ctrlwrite = false;
	u32 I = 0;
	switch( addr )
	{
	case 0x4000102: ctrlwrite = true; I = 0; break;
	case 0x4000106: ctrlwrite = true; I = 1; break;
	case 0x400010A: ctrlwrite = true; I = 2; break;
	case 0x400010E: ctrlwrite = true; I = 3; break;

	case 0x4000100: I = 0; break;
	case 0x4000104: I = 1; break;
	case 0x4000108: I = 2; break;
	case 0x400010C: I = 3; break;
	default:
		std::println("unimpl timer io write ${:X} = ${:X}", addr, v);
		return;
	}
	
	if( ctrlwrite )
	{
		if( (tmr[I].ctrl & 0x82) == 0x80 ) catchup_timer(I); // if timer is running, need to catch it up
		
		v &= 0xc7;
		auto& timer = tmr[I];
		if( timer.ctrl == v ) return;
		u8 oldctrl = timer.ctrl;
		timer.ctrl = v;
		
		// just get rid of any future timer events
		sched.filter_out_event(EVENT_TMR0_CHECK+I);
		
		if( !(oldctrl&0x80) && (timer.ctrl&0x80) )
		{ // gbatek says if start/stop bit changes from 0 to 1, but might be being too literal here
		  // possibly just any write with a bit7=1?
			timer.value = timer.reload;
		}
		
		if( !(timer.ctrl & BIT(7)) || (I!=0 && (timer.ctrl&BIT(2))) ) 
		{
			// if timer is off, or if timers 1,2,or3 
			// count up from prev. overflow then don't do other checks
			return;
		}
		
		u64 ticks_til_overflow = 0x10000 - timer.value; // value, not reload: a ctrl write might not trigger reload
		sched.add_event(cpu.stamp + (ticks_til_overflow * prescaler[timer.ctrl&3]), EVENT_TMR0_CHECK+I);
		timer.last_read = cpu.stamp;
		return;
	}
	
	tmr[I].reload = v;	
	return;
}

u32 gba::read_tmr_io(u32 addr)
{
	//std::println("timer io read ${:X}", addr);
	switch( addr )
	{
	case 0x4000102: return tmr[0].ctrl;
	case 0x4000106: return tmr[1].ctrl;
	case 0x400010A: return tmr[2].ctrl;
	case 0x400010E: return tmr[3].ctrl;

	case 0x4000100: catchup_timer(0); return tmr[0].value;
	case 0x4000104: catchup_timer(1); return tmr[1].value;
	case 0x4000108: catchup_timer(2); return tmr[2].value;
	case 0x400010C: catchup_timer(3); return tmr[3].value;

	default:
		std::println("unimpl timer io read ${:X}", addr);
		return 0xffff;
	}
}

void gba::catchup_timer(u32 I)
{
	auto& timer = tmr[I];
	if( !(timer.ctrl & 0x80) ) return; // don't catchup timers that are off
	if( I!=0 && (timer.ctrl & BIT(2)) ) return; // catchup is for free-running timers
	
	u64 delta = cpu.stamp - timer.last_read;
	delta /= prescaler[timer.ctrl&3];
	if( delta )
	{
		timer.value += delta;
		timer.last_read = cpu.stamp;
	}
}

static int times = 0;

void gba::timer_event(u64 oldstamp, u32 I)
{
	auto& timer = tmr[I];
	u16 old_value = timer.value;
	u64 delta = oldstamp - timer.last_read;
	delta /= prescaler[timer.ctrl&3];
	if( delta )
	{
		timer.value += delta;
		timer.last_read = cpu.stamp;
	}

	if( timer.value < old_value )
	{
		timer.value = timer.reload;
		
		if( timer.ctrl & BIT(6) )
		{
			ISTAT |= BIT(3+I);
			check_irqs();
		}
		
		if( I==0 || !(timer.ctrl&BIT(2)) )
		{
			u64 ticks_til_overflow = 0x10000 - timer.value;
			sched.add_event(oldstamp + (ticks_til_overflow * prescaler[timer.ctrl&3]), EVENT_TMR0_CHECK+I);
			timer.last_read = oldstamp;
		}

		if( I!=3 && (tmr[I+1].ctrl & 0x84) == 0x84 ) tick_overflow_timer(I+1);
		return;
	}

	if( I==0 || !(timer.ctrl&BIT(2)) )
	{
		u64 ticks_til_overflow = 0x10000 - timer.value;
		sched.add_event(oldstamp + (ticks_til_overflow * prescaler[timer.ctrl&3]), EVENT_TMR0_CHECK+I);
		timer.last_read = oldstamp;
	}
}

void gba::tick_overflow_timer(u32 I)
{
	auto& timer = tmr[I];
	
	timer.value += 1;
	if( timer.value ) return;
	
	if( timer.ctrl & BIT(6) )
	{
		ISTAT |= BIT(3+I);
		check_irqs();
	}

	if( I!=3 && (tmr[I+1].ctrl & 0x84) == 0x84 ) tick_overflow_timer(I+1);
}



