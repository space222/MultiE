#include "gbc.h"

static u8 duty[] = { 0,0,0,0,0,0,0,0xF0, 0xF0,0,0,0,0,0,0,0xF0, 0xF0,0,0,0,0,0xF0,0xF0,0xF0, 0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0 };

void gbc::apu_tick()
{
	if( !(io[0x26]&0x80) )
	{
		apu_cycles_to_sample += 1;
		if( apu_cycles_to_sample >= 95 )
		{
			apu_cycles_to_sample = 0;
			audio_add(0, 0);
		}
		return;
	}
	
	u16 total = 0;
	
	if( chan[0].active )
	{
		chan[0].pcnt += 1;
		if( chan[0].pcnt > 0x7ff*4 )
		{
			chan[0].phase = (chan[0].phase + 1)&7;
			chan[0].pcnt = chan[0].period;
		}
		total += (duty[(io[0x11]>>6)*8 + chan[0].phase]>>4) ? (io[0x12]>>4) : 0;
	}
	
	if( chan[1].active )
	{
		chan[1].pcnt += 1;
		if( chan[1].pcnt > 0x7ff*4 )
		{
			chan[1].phase = (chan[1].phase + 1)&7;
			chan[1].pcnt = chan[1].period;
		}
		total += (duty[(io[0x16]>>6)*8 + chan[1].phase]>>4) ? (io[0x17]>>4) : 0;
	}
	
	apu_cycles_to_sample += 1;
	if( apu_cycles_to_sample >= 95 )
	{
		apu_cycles_to_sample = 0;
		float sm = (total/30.f)*2 - 1;
		audio_add(sm, sm);
	}
}

void gbc::apu_write(u8 p, u8 v)
{
	if( p == 0x26 )
	{
		io[0x26] = v;
		return;
	}
	if( !(io[0x26]&0x80) ) return;
	io[p] = v;
	switch( p )
	{
	case 0x12:
		if( !(v & 0xF8) ) chan[0].active = false;	
		break;
	case 0x17:
		if( !(v & 0xF8) ) chan[1].active = false;
		break;
	case 0x14: // chan 1 trigger, length en, period high
		if( v & 0x80 )
		{
			chan[0].active = true;
			chan[0].length = io[0x11]&0x3F;
			chan[0].period = ((io[0x14]&7)<<8)|(io[0x13]);
			chan[0].pcnt = chan[0].period = chan[0].period*4;
		}
		break;
	case 0x19: // chan 2 trigger, length en, period high
		if( v & 0x80 )
		{
			chan[1].active = true;
			chan[1].length = io[0x16]&0x3F;
			chan[1].period = ((io[0x19]&7)<<8)|(io[0x18]);
			chan[1].pcnt = chan[1].period = chan[1].period*4;
		}
		break;
	}
}

void gbc::div_apu_tick()
{
	if( chan[0].active && (io[0x14]&BIT(6)) )
	{
		chan[0].length += 1;
		if( chan[0].length == 128 ) chan[0].active = false;
	}
	if( chan[1].active && (io[0x19]&BIT(6)) )
	{
		chan[1].length += 1;
		if( chan[1].length == 128 ) chan[1].active = false;
	}
}

