#include <print>
#include <array>
#include <cmath>
#include "ym2612.h"

void ym2612::data(u8 v)
{
	//status |= 0x80;
	busy_clear = 2;
	switch( reg )
	{
	case 0x27:{ // mostly timer stuff
		u8 old = R[0x27];
		R[0x27] = v;
		
		if( v & (1<<4) ) { status &= ~1; } //timer A overflow reset
		if( v & (1<<5) ) { status &= ~2; } //timer B overflow reset
		
		if( /*!(old & 1) &&*/ (v&1) ) { timerA = (R[0x24]<<2)|(R[0x25]&3); } //timer A load
		if( /*!(old & 2) &&*/ (v&2) ) { timerB = R[0x26]; timerB_div = 0; } //timer B load

		}break;
	case 0x28:{ // key-on/off
		u32 ch = v&7;
		if( ch == 3 || ch == 7 ) { return; }
		if( ch & 4 ) { ch -= 1; }
		if( v&0x10 )
		{
			if( chan[ch].op[0].adsr == 3 ) { chan[ch].op[0].atten = 0x3ff; chan[ch].op[0].adsr = chan[ch].op[0].phz.counter = 0; }
		} else {
			chan[ch].op[0].adsr = 3;
		}
		if( v&0x20 )
		{
			if( chan[ch].op[1].adsr == 3 ) { chan[ch].op[1].atten = 0x3ff; chan[ch].op[1].adsr = chan[ch].op[1].phz.counter = 0; }
		} else {
			chan[ch].op[1].adsr = 3;
		}
		if( v&0x40 )
		{
			if( chan[ch].op[2].adsr == 3 ) { chan[ch].op[2].atten = 0x3ff; chan[ch].op[2].adsr = chan[ch].op[2].phz.counter = 0; }
		} else {
			chan[ch].op[2].adsr = 3;
		}
		if( v&0x80 )
		{
			if( chan[ch].op[3].adsr == 3 ) { chan[ch].op[3].atten = 0x3ff; chan[ch].op[3].adsr = chan[ch].op[3].phz.counter = 0; }
		} else {
			chan[ch].op[3].adsr = 3;
		}
	}break;
	case 0x30: case 0x31: case 0x32:
		chan[(reg&3) + (last_port?3:0)].op[0].phz.detune = (v>>4)&7;
		chan[(reg&3) + (last_port?3:0)].op[0].phz.mult = v&15;
		break;
	case 0x38: case 0x39: case 0x3A:
		chan[(reg&3) + (last_port?3:0)].op[1].phz.detune = (v>>4)&7;
		chan[(reg&3) + (last_port?3:0)].op[1].phz.mult = v&15;
		break;
	case 0x34: case 0x35: case 0x36:
		chan[(reg&3) + (last_port?3:0)].op[2].phz.detune = (v>>4)&7;
		chan[(reg&3) + (last_port?3:0)].op[2].phz.mult = v&15;
		break;
	case 0x3C: case 0x3D: case 0x3E:
		chan[(reg&3) + (last_port?3:0)].op[3].phz.detune = (v>>4)&7;
		chan[(reg&3) + (last_port?3:0)].op[3].phz.mult = v&15;
		break;

	case 0x40: case 0x41: case 0x42:
		chan[(reg&3) + (last_port?3:0)].op[0].TL = v&0x7f;
		break;
	case 0x48: case 0x49: case 0x4A:
		chan[(reg&3) + (last_port?3:0)].op[1].TL = v&0x7f;
		break;
	case 0x44: case 0x45: case 0x46:
		chan[(reg&3) + (last_port?3:0)].op[2].TL = v&0x7f;
		break;
	case 0x4C: case 0x4D: case 0x4E:
		chan[(reg&3) + (last_port?3:0)].op[3].TL = v&0x7f;
		break;

	case 0x50: case 0x51: case 0x52:
		chan[(reg&3) + (last_port?3:0)].op[0].key_scale = (v>>6)&3;
		chan[(reg&3) + (last_port?3:0)].op[0].rates[0] = v&31;
		break;
	case 0x58: case 0x59: case 0x5A:
		chan[(reg&3) + (last_port?3:0)].op[1].key_scale = (v>>6)&3;
		chan[(reg&3) + (last_port?3:0)].op[1].rates[0] = v&31;
		break;
	case 0x54: case 0x55: case 0x56:
		chan[(reg&3) + (last_port?3:0)].op[2].key_scale = (v>>6)&3;
		chan[(reg&3) + (last_port?3:0)].op[2].rates[0] = v&31;
		break;
	case 0x5C: case 0x5D: case 0x5E:
		chan[(reg&3) + (last_port?3:0)].op[3].key_scale = (v>>6)&3;
		chan[(reg&3) + (last_port?3:0)].op[3].rates[0] = v&31;
		break;

	case 0x60: case 0x61: case 0x62:
		chan[(reg&3) + (last_port?3:0)].op[0].rates[1] = v&31;
		break;
	case 0x68: case 0x69: case 0x6A:
		chan[(reg&3) + (last_port?3:0)].op[1].rates[1] = v&31;
		break;
	case 0x64: case 0x65: case 0x66:
		chan[(reg&3) + (last_port?3:0)].op[2].rates[1] = v&31;
		break;
	case 0x6C: case 0x6D: case 0x6E:
		chan[(reg&3) + (last_port?3:0)].op[3].rates[1] = v&31;
		break;

	case 0x70: case 0x71: case 0x72:
		chan[(reg&3) + (last_port?3:0)].op[0].rates[2] = v&31;
		break;
	case 0x78: case 0x79: case 0x7A:
		chan[(reg&3) + (last_port?3:0)].op[1].rates[2] = v&31;
		break;
	case 0x74: case 0x75: case 0x76:
		chan[(reg&3) + (last_port?3:0)].op[2].rates[2] = v&31;
		break;
	case 0x7C: case 0x7D: case 0x7E:
		chan[(reg&3) + (last_port?3:0)].op[3].rates[2] = v&31;
		break;

	case 0x80: case 0x81: case 0x82:
		chan[(reg&3) + (last_port?3:0)].op[0].suslevel = v>>4;
		chan[(reg&3) + (last_port?3:0)].op[0].rates[3] = v&15;
		break;
	case 0x88: case 0x89: case 0x8A:
		chan[(reg&3) + (last_port?3:0)].op[1].suslevel = v>>4;
		chan[(reg&3) + (last_port?3:0)].op[1].rates[3] = v&15;
		break;
	case 0x84: case 0x85: case 0x86:
		chan[(reg&3) + (last_port?3:0)].op[2].suslevel = v>>4;
		chan[(reg&3) + (last_port?3:0)].op[2].rates[3] = v&15;
		break;
	case 0x8C: case 0x8D: case 0x8E:
		chan[(reg&3) + (last_port?3:0)].op[3].suslevel = v>>4;
		chan[(reg&3) + (last_port?3:0)].op[3].rates[3] = v&15;
		break;

		
	case 0xA0:
		chan[0+(last_port?3:0)].fnum = ((R[0xA4]&7)<<8)|v;
		chan[0+(last_port?3:0)].block = (R[0xA4]>>3)&7;
		break;
	case 0xA1:
		chan[1+(last_port?3:0)].fnum = ((R[0xA5]&7)<<8)|v;
		chan[1+(last_port?3:0)].block = (R[0xA5]>>3)&7;
		break;
	case 0xA2:
		chan[2+(last_port?3:0)].fnum = ((R[0xA6]&7)<<8)|v;
		chan[2+(last_port?3:0)].block = (R[0xA6]>>3)&7;
		if( !last_port ) { chan[2].op[3].phz.fnum=((R[0xA6]&7)<<8)|v; chan[2].op[3].phz.block=(R[0xA6]>>3)&7; }
		break;
	
	case 0xA9:
		chan[2].op[0].phz.fnum = ((R[0xAD]&7)<<8)|v;
		chan[2].op[0].phz.block = (R[0xAD]>>3)&7;
		break;
	case 0xAA:
		chan[2].op[1].phz.fnum = ((R[0xAE]&7)<<8)|v;
		chan[2].op[1].phz.block = (R[0xAE]>>3)&7;
		break;
	case 0xA8:
		chan[2].op[2].phz.fnum = ((R[0xAC]&7)<<8)|v;
		chan[2].op[2].phz.block = (R[0xAC]>>3)&7;
		break;
	
		
	case 0xB0:
	case 0xB1:
	case 0xB2:
		chan[(reg&3)+(last_port?3:0)].algo=v&7;
		chan[(reg&3)+(last_port?3:0)].feedback=(v>>3)&7;		
		break;
	default:
		R[reg] = v;
		break;
	}
}

void ym2612::ctrl(u8 p, u8 v)
{
	last_port = p;
	reg = v;
}

u32 get_keycode(u32 bloc, u32 fnum)
{
	u32 KC = (bloc<<2);
	KC |= (fnum>>10)&2;
	u32 F11 = (fnum>>11)&1;
	u32 F10 = (fnum>>10)&1;
	u32 F9 = (fnum>>9)&1;
	u32 F8 = (fnum>>8)&1;
	KC |= (F11 & (F10 | F9 | F8)) | ((F11^1) & F10 & F9 & F8); // from jsgroth blog
	return KC;
}

void ym2612::op_phase(u32 ch, u32 oper, u32 phase_add)
{
	auto& C = chan[ch];
	// calc initial increment
	u32 fnum = ((ch==2 && (R[0x27]&0xc0)) ? C.op[oper].phz.fnum : C.fnum);
	u32 block= ((ch==2 && (R[0x27]&0xc0)) ? C.op[oper].phz.block : C.block);
	u32 inc = (fnum << block)>>1;
	// apply detune based on weird 'keycode'
	u32 kc = get_keycode(C.block, C.fnum);
	u32 det = detune[kc*4 + (C.op[oper].phz.detune&3)];
	if( C.op[oper].phz.detune&4 ) { inc -= det; } else { inc += det; }
	inc &= 0x1FFFF;
	// multiplier
	if( C.op[oper].phz.mult )
	{
		inc *= C.op[oper].phz.mult;
	} else {
		inc >>= 1;
	}
	// do the inc
	C.op[oper].phz.counter += inc;
	C.op[oper].phz.counter &= 0xFFFFF;
	// pipeline the old output, calc new output + incoming phase shift
	C.op[oper].phz.older = C.op[oper].phz.old;
	C.op[oper].phz.old = C.op[oper].phz.out;
	// calculate the output including any incoming modulation
	int phase = C.op[oper].phz.counter >> 10;
	phase += (int(phase_add) >> 1) & 0x3FF;
	float S = sin(phase / 1024.f * 2.0f * 3.14159265359f);
	float atten = std::exp2(-((float)C.op[oper].level() / 64.0f));
	C.op[oper].phz.out = int(S * atten * 8192.0f);
}

#define OP0FEEDBACK (chan[i].feedback ? ((int(chan[i].op[0].phz.out) + int(chan[i].op[0].phz.old)) >> (9 - chan[i].feedback)) : 0)

void ym2612::cycle()
{
	env_clkdiv += 1;
	if( env_clkdiv >= 144*3 )
	{
		if( busy_clear )
		{
			busy_clear -= 1;
			if( busy_clear == 0 )
			{
				status &= 0x7f;	
			}
		}
	
		env_clkdiv = 0;
		env_clock += 1;
		if( env_clock >= (1<<12) )
		{
			env_clock = 1;
		}
		for(u32 ch = 0; ch < 6; ++ch)
		{
			for(u32 o = 0; o < 4; ++o)
			{
				auto& OP = chan[ch].op[o];
				if( OP.adsr == 0 && OP.atten == 0 )
				{
					OP.adsr = 1;
				}
				
				u32 sl_steps = (OP.suslevel == 15 ? (0x3FF>>5) : OP.suslevel);
				u32 SL = sl_steps<<5;
				if( OP.adsr == 1 && OP.atten >= SL )
				{
					OP.adsr = 2;
				}
				
				u32 Rks = get_keycode(chan[ch].block, chan[ch].fnum) >> (3-OP.key_scale);
				u32 R = OP.rates[OP.adsr];
				if( OP.adsr == 3 ) { R = 2*R + 1; }
				u32 Rate = (R ? 2*R+Rks : 0);
				if( Rate > 63 ) { Rate = 63; }
				
				u32 shift = (Rate >= 44 ? 0 : 11-(Rate/4));
				if( !(env_clock & ((1 << shift)-1)) ) 
				{
					u32 index = (env_clock >> shift)&7;
					int incr = envtable[Rate*8 + index];
					if( OP.adsr )
					{
						OP.atten = std::min<u32>(OP.atten+incr, 0x3FF);
					} else {
						OP.atten = std::max<int>(0, int(OP.atten) + ((incr * -(int(OP.atten) + 1)) >> 4));
					}
				}
			}
		}
	}

	phase_clkdiv += 1;
	if( phase_clkdiv >= 144 )
	{
		phase_clkdiv = 0;
		currentL = 0;
		
		run_timers();
		
		for(u32 i = 0; i < 6; ++i)
		{
			if( i == 5 && (R[0x2B] & 0x80) )
			{ // pcm replaced channel 6 output
			  // technically it should still run, dunno if anything bothers to do something like that
				currentL += ((((int)R[0x2A])-128)/128.f) * 0.5f; // 0.5 to reduce the pcm a bit
				break;
			}
		
			// I don't really understand what should get current or old output
			switch( chan[i].algo )
			{
			case 0:{
				op_phase(i, 0, OP0FEEDBACK);
				op_phase(i, 1, chan[i].op[0].phz.old);
				op_phase(i, 2, chan[i].op[1].phz.old);
				op_phase(i, 3, chan[i].op[2].phz.old);
				currentL += (int(chan[i].op[3].phz.out)/32768.f);
				}break;
			case 1:{
				op_phase(i, 0, OP0FEEDBACK);
				op_phase(i, 1);
				op_phase(i, 2, chan[i].op[0].phz.old+chan[i].op[1].phz.old);
				op_phase(i, 3, chan[i].op[2].phz.old);
				currentL += (int(chan[i].op[3].phz.out)/32768.f);
				}break;			
			case 2:{
				op_phase(i, 0, OP0FEEDBACK);
				op_phase(i, 1);
				op_phase(i, 2, chan[i].op[1].phz.old);
				op_phase(i, 3, chan[i].op[0].phz.old+chan[i].op[2].phz.old);
				currentL += (int(chan[i].op[3].phz.out)/32768.f);
				}break;
			case 3:{
				op_phase(i, 0, OP0FEEDBACK);
				op_phase(i, 1, chan[i].op[0].phz.old);
				op_phase(i, 2);
				op_phase(i, 3, chan[i].op[1].phz.old+chan[i].op[2].phz.old);
				currentL += (int(chan[i].op[3].phz.out)/32768.f);
				}break;
			case 4:{
				op_phase(i, 0, OP0FEEDBACK);
				op_phase(i, 2);
				op_phase(i, 1, chan[i].op[0].phz.old);
				op_phase(i, 3, chan[i].op[2].phz.old);
				int total = chan[i].op[1].phz.out+chan[i].op[3].phz.out;			      
				currentL += (total/32768.f);
				}break;
			case 5:{
				op_phase(i, 0, OP0FEEDBACK);
				op_phase(i, 1, chan[i].op[0].phz.old);
				op_phase(i, 2, chan[i].op[0].phz.old);
				op_phase(i, 3, chan[i].op[0].phz.old);
				int total = chan[i].op[1].phz.out+chan[i].op[2].phz.out+chan[i].op[3].phz.out;			      
				currentL += (total/32768.f);
				}break;
			case 6:{
				op_phase(i, 0, OP0FEEDBACK);
				op_phase(i, 2);
				op_phase(i, 1, chan[i].op[0].phz.old);
				op_phase(i, 3);
				int total =chan[i].op[1].phz.out+chan[i].op[2].phz.out+chan[i].op[3].phz.out;			      
				currentL += (total/32768.f);
				}break;
			case 7:{
				op_phase(i, 0, OP0FEEDBACK);
				op_phase(i, 1);
				op_phase(i, 2);
				op_phase(i, 3);
				int total = chan[i].op[0].phz.out+chan[i].op[1].phz.out+chan[i].op[2].phz.out+chan[i].op[3].phz.out;
				currentL += (total/32768.f);
				}break;
			}
		}
	}
	stamp += 1;	
}

#define TMR_A_OVERFLOW_EN (1<<2)
#define TMR_B_OVERFLOW_EN (1<<3)

#define TMR_A_EN 1
#define TMR_B_EN 2

void ym2612::run_timers()
{
	if( R[0x27] & TMR_A_EN ) //??
	{
		timerA += 1;
		timerA &= 0x3FF;
		if( timerA == 0 )
		{
			timerA = (R[0x24]<<2)|(R[0x25]&3);
			if( R[0x27] & TMR_A_OVERFLOW_EN )
			{
				status |= 1;
			}
		}
	}
	
	if( R[0x27] & TMR_B_EN )
	{
		timerB_div += 1;
		if( timerB_div >= 16 )
		{
			timerB_div = 0;
			timerB += 1;
			timerB &= 0xff;
			if( timerB == 0 )
			{
				timerB = R[0x26];
				if( R[0x27] & TMR_B_OVERFLOW_EN )
				{
					status |= 2;
				}
			}		
		}
	}
}

void ym2612::reset()
{
	for(u32 i = 0; i < 6; ++i)
	{
		chan[i].op[0].adsr = chan[i].op[1].adsr = chan[i].op[2].adsr = chan[i].op[3].adsr = 3;
		chan[i].op[0].atten = chan[i].op[1].atten = chan[i].op[2].atten = chan[i].op[3].atten = 0x3ff;
	}
	status = 0;
}

u32 ym2612::detune[] = {
    0,  0,  1,  2,  0,  0,  1,  2,  0,  0,  1,  2,  0,  0,  1,  2,  // Block 0
    0,  1,  2,  2,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  // Block 1
    0,  1,  2,  4,  0,  1,  3,  4,  0,  1,  3,  4,  0,  1,  3,  5,  // Block 2
    0,  2,  4,  5,  0,  2,  4,  6,  0,  2,  4,  6,  0,  2,  5,  7,  // Block 3
    0,  2,  5,  8,  0,  3,  6,  8,  0,  3,  6,  9,  0,  3,  7, 10,  // Block 4
    0,  4,  8, 11,  0,  4,  8, 12,  0,  4,  9, 13,  0,  5, 10, 14,  // Block 5
    0,  5, 11, 16,  0,  6, 12, 17,  0,  6, 13, 19,  0,  7, 14, 20,  // Block 6
    0,  8, 16, 22,  0,  8, 16, 22,  0,  8, 16, 22,  0,  8, 16, 22,  // Block 7
};

u32 ym2612::envtable[] = {
   0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,1,0,1,0,1,0,1, 0,1,0,1,0,1,0,1,  // 0-3
    0,1,0,1,0,1,0,1, 0,1,0,1,0,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,0,1,1,1,  // 4-7
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 8-11
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 12-15
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 16-19
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 20-23
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 24-27
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 28-31
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 32-35
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 36-39
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 40-43
    0,1,0,1,0,1,0,1, 0,1,0,1,1,1,0,1, 0,1,1,1,0,1,1,1, 0,1,1,1,1,1,1,1,  // 44-47
    1,1,1,1,1,1,1,1, 1,1,1,2,1,1,1,2, 1,2,1,2,1,2,1,2, 1,2,2,2,1,2,2,2,  // 48-51
    2,2,2,2,2,2,2,2, 2,2,2,4,2,2,2,4, 2,4,2,4,2,4,2,4, 2,4,4,4,2,4,4,4,  // 52-55
    4,4,4,4,4,4,4,4, 4,4,4,8,4,4,4,8, 4,8,4,8,4,8,4,8, 4,8,8,8,4,8,8,8,  // 56-59
    8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,  // 60-63
};

