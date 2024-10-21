#include "gbc.h"

static u8 duty[] = { 0,0,0,0,0,0,0,0xF0, 0xF0,0,0,0,0,0,0,0xF0, 0xF0,0,0,0,0,0xF0,0xF0,0xF0, 0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0 };
static u8 chan3shift[] = { 8, 0, 1, 2 };

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
		if( io[0x25] & 0x11 )
			total += (duty[(io[0x11]>>6)*8 + chan[0].phase]>>4) ? chan[0].env : 0;
	}
	
	if( chan[1].active )
	{
		chan[1].pcnt += 1;
		if( chan[1].pcnt > 0x7ff*4 )
		{
			chan[1].phase = (chan[1].phase + 1)&7;
			chan[1].pcnt = chan[1].period;
		}
		if( io[0x25] & 0x22 )
			total += (duty[(io[0x16]>>6)*8 + chan[1].phase]>>4) ? chan[1].env : 0;
	}
	
	if( chan[2].active )
	{
		chan[2].pcnt += 1;
		if( chan[2].pcnt > 0x7ff*4 )
		{
			chan[2].phase = (chan[2].phase + 1)&0x1F;
			chan[2].pcnt = chan[2].period;
		}
		if( io[0x25] & 0x44 )
			total += ((io[0x30 + (chan[2].phase>>1)] >> (((chan[2].phase&1)^1)*4))&0xf) >> chan3shift[(io[0x1C]>>5)&3];
	}
	
	if( chan[3].active )
	{
		chan[3].pcnt -= 1;
		if( chan[3].pcnt == 0 )
		{
			u16 nv = ((noise_lfsr)^(noise_lfsr>>1))&1;
			nv ^= 1;
			noise_lfsr >>= 1;
			noise_lfsr |= nv<<15;
			if( io[0x22] & 4 ) noise_lfsr = (noise_lfsr&~BIT(6))|(nv<<6);
			chan[3].pcnt = chan[3].period;
		}
		if( io[0x25] & 0x88 )
		{
			total += (noise_lfsr&1) ? chan[3].env : 0;
		}
	}
	
	apu_cycles_to_sample += 1;
	if( apu_cycles_to_sample >= 95 )
	{
		apu_cycles_to_sample = 0;
		float sm = (total/60.f)*2 - 1;
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
	case 0x21:
		//printf("noise vol/env = $%X\n", v);
		if( !(v & 0xF8) ) chan[3].active = false;
		break;
	case 0x1A:
		if( !(v&0x80) ) chan[2].active = false;
		break;
	case 0x12:
		if( !(v & 0xF8) ) chan[0].active = false;	
		break;
	case 0x17:
		if( !(v & 0xF8) ) chan[1].active = false;
		break;
	case 0x14: // chan 1 trigger, length en, period high
		if( (v & 0x80) && (io[0x12]&0xf8) )
		{
			chan[0].active = true;
			chan[0].length = (io[0x11]&0x3F)*2;
			chan[0].period = ((io[0x14]&7)<<8)|(io[0x13]);
			chan[0].pcnt = chan[0].period = chan[0].period*4;
			chan[0].env = (io[0x12]>>4);
			chan[0].envcnt = 0;
		}
		break;
	case 0x19: // chan 2 trigger, length en, period high
		if( (v & 0x80) && (io[0x17]&0xf8) )
		{
			chan[1].active = true;
			chan[1].length = (io[0x16]&0x3F)*2;
			chan[1].period = ((io[0x19]&7)<<8)|(io[0x18]);
			chan[1].pcnt = chan[1].period = chan[1].period*4;
			chan[1].env = (io[0x17]>>4);
			chan[1].envcnt = 0;
		}
		break;
	case 0x1E: // chan 3 trigger, length en, period high
		if( (v & 0x80) && (io[0x1A]&0x80) )
		{
			chan[2].active = true;
			chan[2].phase = 0;
			chan[2].length = io[0x1B]*2;
			chan[2].period = ((io[0x1E]&7)<<8)|(io[0x1D]);
			chan[2].pcnt = chan[2].period = chan[2].period*4;
		}	
		break;
	case 0x23: // chan 4 trigger, length en (no period bits)
		if( (v & 0x80) && (io[0x21]&0xf8) )
		{
			chan[3].active = true;
			chan[3].length = (io[0x20]&0x3F)*2;
			chan[3].period = (io[0x22]&7) ? (io[0x22]&7)*16 : 8;
			chan[3].period <<= (io[0x22]>>4)+1;
			chan[3].pcnt = chan[3].period;
			chan[3].env = io[0x21] >> 4;
			chan[3].envcnt = 0;
			noise_lfsr = 0x7fff;
		}
		break;
	}
}

void gbc::div_apu_tick()
{
	if( chan[0].active )
	{
		if( (io[0x14]&BIT(6)) )
		{
			chan[0].length += 1;
			if( chan[0].length >= 128 ) chan[0].active = false;
		}
		chan[0].envcnt += 1;
		if( chan[0].envcnt >= (io[0x12]&7)*4 )
		{
			chan[0].envcnt = 0;
			if( io[0x12] & BIT(3) )
			{
				if( chan[0].env < 0xf ) chan[0].env += 1;
			} else {
				if( chan[0].env != 0 ) chan[0].env -= 1;
			}
		}
	}
	if( chan[1].active )
	{
		if( (io[0x19]&BIT(6)) )
		{
			chan[1].length += 1;
			if( chan[1].length >= 128 ) chan[1].active = false;
		}
		chan[1].envcnt += 1;
		if( chan[1].envcnt >= (io[0x17]&7)*4 )
		{
			chan[1].envcnt = 0;
			if( io[0x17] & BIT(3) )
			{
				if( chan[1].env < 0xf ) chan[1].env += 1;
			} else {
				if( chan[1].env != 0 ) chan[1].env -= 1;
			}
		}
	}
	if( chan[2].active )
	{
		if( (io[0x1E]&BIT(6)) )
		{
			chan[2].length += 1;
			if( chan[2].length >= 512 ) chan[2].active = false;
		}
	}
	if( chan[3].active )
	{
		if( (io[0x23]&BIT(6)) )
		{
			chan[3].length += 1;
			if( chan[3].length >= 128 ) chan[3].active = false;
		}
		chan[3].envcnt += 1;
		if( chan[3].envcnt >= (io[0x21]&7)*4 )
		{
			chan[3].envcnt = 0;
			if( io[0x21] & BIT(3) )
			{
				if( chan[3].env < 0xf ) chan[3].env += 1;
			} else {
				if( chan[3].env != 0 ) chan[3].env -= 1;
			}
		}
	}
}

