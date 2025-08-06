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
		if( (tmr[I].ctrl & 0x84) == 0x80 ) catchup_timer(I); // if timer is running, need to catch it up
		
		v &= 0xc7;
		auto& timer = tmr[I];
		if( timer.ctrl == v ) return;
		u8 oldctrl = timer.ctrl;
		timer.ctrl = v;
		
		// just get rid of any future timer events
		sched.filter_out_event(EVENT_TMR0_CHECK+I);
		
		const bool started = !(oldctrl&0x80) && (timer.ctrl&0x80);
		
		if( started )
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
		
		u64 cycles_til_overflow = (0x10000 - timer.value) * prescaler[timer.ctrl&3];
				// ^value, not reload: a ctrl write might not trigger reload
		if( cycles_til_overflow == 0 ) cycles_til_overflow += 5;
		sched.add_event(cpu.stamp + cycles_til_overflow, EVENT_TMR0_CHECK+I);
		timer.last_read = cpu.stamp;
		return;
	}
	
	tmr[I].reload = v;
	return;
}

u32 gba::read_tmr_io(u32 addr)
{
	auto timer_val = [&](u32 I)->u16
	{
		if( !(tmr[I].ctrl & BIT(7)) ) return tmr[I].value;
		if( I!=0 && (tmr[I].ctrl & BIT(2)) ) return tmr[I].value;
		return tmr[I].value + (cpu.stamp - tmr[I].last_read)/prescaler[tmr[I].ctrl&3];
	};


	//std::println("timer io read ${:X}", addr);
	switch( addr )
	{
	case 0x4000102: return tmr[0].ctrl;
	case 0x4000106: return tmr[1].ctrl;
	case 0x400010A: return tmr[2].ctrl;
	case 0x400010E: return tmr[3].ctrl;

	case 0x4000100: return timer_val(0);
	case 0x4000104: return timer_val(1);
	case 0x4000108: return timer_val(2);
	case 0x400010C: return timer_val(3);

	default:
		std::println("unimpl timer io read ${:X}", addr);
		return 0xffff;
	}
}

void gba::catchup_timer(u32 I)
{ // currently unused
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

void gba::timer_event(u64 oldstamp, u32 I)
{
	auto& timer = tmr[I];
	timer.value = timer.reload;
		
	if( snd_master_en )
	{
		if( (dma_sound_mix&0x0300) && I==((dma_sound_mix>>10)&1) ) snd_a_timer();
		if( (dma_sound_mix&0x3000) && I==((dma_sound_mix>>14)&1) ) snd_b_timer();
	}
	
	if( timer.ctrl & BIT(6) )
	{
		ISTAT |= BIT(3+I);
		check_irqs();
	}
	
	if( I==0 || !(timer.ctrl&BIT(2)) )
	{
		u64 cycles_til_overflow = (0x10000 - timer.value) * prescaler[timer.ctrl&3];
		if( cycles_til_overflow == 0 ) cycles_til_overflow += 5;
		sched.add_event(oldstamp + cycles_til_overflow, EVENT_TMR0_CHECK+I);
		timer.last_read = oldstamp;
	}

	if( I!=3 && (tmr[I+1].ctrl & 0x84) == 0x84 ) tick_overflow_timer(I+1);
}

void gba::tick_overflow_timer(u32 I)
{
	auto& timer = tmr[I];
	
	timer.value += 1;
	if( timer.value ) return;
	
	timer.value = timer.reload;
	
	if( timer.ctrl & BIT(6) )
	{
		ISTAT |= BIT(3+I);
		check_irqs();
	}
	
	if( I<2 && snd_master_en )
	{
		if( (dma_sound_mix&0x0300) && I==((dma_sound_mix>>10)&1) ) snd_a_timer();
		if( (dma_sound_mix&0x3000) && I==((dma_sound_mix>>14)&1) ) snd_b_timer();
	}

	if( I!=3 && (tmr[I+1].ctrl & 0x84) == 0x84 ) tick_overflow_timer(I+1);
}



