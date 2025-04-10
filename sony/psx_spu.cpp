#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "psx.h"

#define LOOP_END    (SRAM[V.curaddr+1]&1)
#define LOOP_REPEAT (SRAM[V.curaddr+1]&2)
#define LOOP_START  (SRAM[V.curaddr+1]&4)
#define ENVELOPE_COUNTER_MAX (1u << (33 - 11))


extern s16 spu_interp_table[]; // not really extern, just forward declared
static int pos_xa_adpcm_table[] = { 0, 60, 115, 98, 122 };
static int neg_xa_adpcm_table[] = { 0, 0, -52, -55, -60 };

void psx::tick_spu()
{
	float total = 0;
	for(u32 v = 0; v < 24; ++v)
	{
		auto& V = svoice[v];

		// handle key on/off
		if( spu_kon & (1u<<v) )
		{
			spu_kon &= ~(1u<<v);
			V.curaddr = V.start&0x7ffff;
			V.adsr_state = ADSR_ATTACK;
			V.pcount = V.curnibble = V.adsr_vol = 0;
			V.adsr_cyc = ENVELOPE_COUNTER_MAX;
			spu_decode_block(v);
		}
		if( spu_koff & (1u<<v) )
		{
			spu_koff &= ~(1u<<v);
			V.adsr_state = ADSR_RELEASE;	
			V.adsr_cyc = ENVELOPE_COUNTER_MAX;
		}

		// move through the samples
		V.pcount += ( (V.vxpitch>0x4000) ? 0x4000 : V.vxpitch );	
		while( V.pcount >= 0x1000 )
		{
			V.pcount -= 0x1000;
			V.curnibble += 1;
			if( V.curnibble >= 28 )
			{
				V.curnibble = 0;
				spu_decode_block(v);
			}
		}

		V.out = V.decoded_samples[V.curnibble];
		
		s32 ev = V.out;
		ev *= (s32(V.Rvol)+s32(V.Lvol))/2;
		if( V.adsr_state != ADSR_RELEASE )
		{
			total += (((u16(V.out)^0x8000)/65535.f)*2 - 1);
		}
	}
	
	spu_out = std::clamp(total/4, -1.f, 1.f);
}

void psx::spu_decode_block(u32 voice)
{
	auto& V = svoice[voice];
	V.curaddr &= 0x7ffff;
	u32 shf = SRAM[V.curaddr]&0xf;
	shf = (shf > 12) ? 9 : shf;
	u32 filter = (SRAM[V.curaddr]>>4)&7;
	if( filter > 4 ) filter = 4;
	s16 f0 = pos_xa_adpcm_table[filter];
	s16 f1 = neg_xa_adpcm_table[filter];

	for(u32 i = 0; i < 28; ++i)
	{
		int t = SRAM[V.curaddr + 2 + (i>>1)];
		if( i & 1 ) t >>= 4;
		else t &= 0xf;
		t = s16(t<<12)>>12;
		//t <<= 12;
		//t >>= shift;
		t <<= 12 - shf;
		
		int samp = t + ((V.old*f0 + V.older*f1+32)/64);
  		V.cur = std::clamp(samp, -32768, 32767);
		V.oldest = V.older;
		V.older = V.old;
		V.old = V.cur;
        	V.decoded_samples[i] = V.cur;
	}

	if( LOOP_START )
	{
	 	V.repeat = V.curaddr;
	}

	if( LOOP_END )
	{
		V.curaddr = V.repeat;	

		if( ! LOOP_REPEAT )
		{
			V.adsr_state = ADSR_RELEASE;
			V.adsr_vol = 0;
		}
	} else {
		V.curaddr += 16;
	}
}

void psx::spu_write(u32 addr, u32 val, int size)
{
	if( size != 16 )
	{
		printf("SPU: Write%i $%X = $%X\n", size, addr, val);
		exit(1);
	}
	
	if( addr < 0x1F801D80 )
	{
		u32 V = (addr>>4)&0x1f;
		u32 reg = (addr&0xf);
		switch( reg )
		{
		case 0: svoice[V].Lvol = val; return;
		case 2: svoice[V].Rvol = val; return;
		case 4: svoice[V].vxpitch = val&0xffff; return;
		case 6: svoice[V].start = (val&0xffff)<<3; return;
		case 8: svoice[V].adsr = val&0xffff; return;
		case 0xA: svoice[V].sustain = val&0xffff; return;
		case 0xC: return; // adsr current volume?
		case 0xE: svoice[V].repeat = (val&0xffff)<<3; return;
		default:
			printf("SPU: voice reg %i = $%X\n", reg, val);
			exit(1);
		}		
		return;
	}
	
	switch( addr )
	{
	case 0x1F801DA6: spu_write_addr = (val&0xffff)<<3; return;
	case 0x1F801D80: spu_mainvol_L = val; return;
	case 0x1F801D82: spu_mainvol_R = val; return;
	case 0x1F801D84: spu_reverb_outvol_L = val; return;
	case 0x1F801D86: spu_reverb_outvol_R = val; return;
	case 0x1F801DAA: spu_ctrl = val; return;
	case 0x1F801D88: spu_kon &= 0xff0000; spu_kon |= (val&0xffff); break;
	case 0x1F801D8A: spu_kon &= 0xffff; spu_kon |= (val&0xff)<<16; break;
	case 0x1F801D8C: spu_koff &= 0xff0000; spu_koff |= (val&0xffff); return;
	case 0x1F801D8E: spu_koff &= 0xffff; spu_koff |= (val&0xff)<<16; return;
	case 0x1F801D90: spu_pmon &= 0xff0000; spu_pmon |= (val&0xffff); return;
	case 0x1F801D92: spu_pmon &= 0xffff; spu_pmon |= (val&0xff)<<16; return;
	case 0x1F801D94: spu_en_noise&=0xff0000; spu_en_noise|=(val&0xffff); return;
	case 0x1F801D96: spu_en_noise&=0xffff; spu_en_noise|=(val&0xff)<<16; return;
	case 0x1F801D98: spu_en_reverb&=0xff0000; spu_en_reverb|=(val&0xffff); return;
	case 0x1F801D9A: spu_en_reverb&=0xffff; spu_en_reverb|=(val&0xff)<<16; return;
	case 0x1F801DB0: spu_cdvol_L = val; return;
	case 0x1F801DB2: spu_cdvol_R = val; return;
	case 0x1F801DB4:
	case 0x1F801DB6: return; // external audio?
	case 0x1F801DAC: spu_tran_ctrl = val; return;
	case 0x1F801DA8: SRAM[spu_write_addr&0x7ffff] = val; spu_write_addr+=1; 
			 SRAM[spu_write_addr&0x7ffff] = val>>8; spu_write_addr+=1; 
			 return;
			 
	case 0x1F801DA2: spu_reverb_start = (val&0xffff)<<3; return;
	case 0x1F801DC0: return;
	default: break;	
	}
		
	//printf("SPU: W$%X = $%X\n", addr, val);
	return;
}

u32 psx::spu_read(u32 addr, int size)
{
	if( size == 8 )
	{
		return spu_read(addr&~1, 16)>>((addr&1)*8);
	}
	if( size == 32 )
	{
		u32 r = spu_read(addr, 16);
		r |= spu_read(addr+2, 16)<<16;
		return r;
	}
	
	//if( addr == 0x1F801D98 ) return 0xffff;

	if( addr == 0x1F801DAA ) return spu_ctrl;
	if( addr == 0x1F801DAE ) return spu_ctrl&0x3F; //0x10;
	if( addr == 0x1F801DAC ) return spu_tran_ctrl;
	if( addr == 0x1F801DA6 ) return spu_write_addr>>3;

	if( addr < 0x1F801D80 )
	{
		u32 V = (addr>>4)&0x1f;
		u32 reg = (addr&0xf);
		switch( reg )
		{
		case 0xC: return svoice[V].adsr_vol; // adsr current volume?
		default: 
			//printf("SPU: Read Voice %i, reg $%X\n", V, reg);
			break;	
		}		
		return 0;
	}

	//printf("SPU: Read $%X\n", addr);
	return 0;
}

s16 spu_interp_table[] = {
-1, -1, -1, -1, -1, -1, -1, -1,
-1, -1, -1, -1, -1, -1, -1, -1,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001,
0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0002, 0x0003, 0x0003,
0x0003, 0x0004, 0x0004, 0x0005, 0x0005, 0x0006, 0x0007, 0x0007,
0x0008, 0x0009, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E,
0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0015, 0x0016, 0x0018,
0x0019, 0x001B, 0x001C, 0x001E, 0x0020, 0x0021, 0x0023, 0x0025,
0x0027, 0x0029, 0x002C, 0x002E, 0x0030, 0x0033, 0x0035, 0x0038,
0x003A, 0x003D, 0x0040, 0x0043, 0x0046, 0x0049, 0x004D, 0x0050,
0x0054, 0x0057, 0x005B, 0x005F, 0x0063, 0x0067, 0x006B, 0x006F,
0x0074, 0x0078, 0x007D, 0x0082, 0x0087, 0x008C, 0x0091, 0x0096,
0x009C, 0x00A1, 0x00A7, 0x00AD, 0x00B3, 0x00BA, 0x00C0, 0x00C7,
0x00CD, 0x00D4, 0x00DB, 0x00E3, 0x00EA, 0x00F2, 0x00FA, 0x0101,
0x010A, 0x0112, 0x011B, 0x0123, 0x012C, 0x0135, 0x013F, 0x0148,
0x0152, 0x015C, 0x0166, 0x0171, 0x017B, 0x0186, 0x0191, 0x019C,
0x01A8, 0x01B4, 0x01C0, 0x01CC, 0x01D9, 0x01E5, 0x01F2, 0x0200,
0x020D, 0x021B, 0x0229, 0x0237, 0x0246, 0x0255, 0x0264, 0x0273,
0x0283, 0x0293, 0x02A3, 0x02B4, 0x02C4, 0x02D6, 0x02E7, 0x02F9,
0x030B, 0x031D, 0x0330, 0x0343, 0x0356, 0x036A, 0x037E, 0x0392,
0x03A7, 0x03BC, 0x03D1, 0x03E7, 0x03FC, 0x0413, 0x042A, 0x0441,
0x0458, 0x0470, 0x0488, 0x04A0, 0x04B9, 0x04D2, 0x04EC, 0x0506,
0x0520, 0x053B, 0x0556, 0x0572, 0x058E, 0x05AA, 0x05C7, 0x05E4,
0x0601, 0x061F, 0x063E, 0x065C, 0x067C, 0x069B, 0x06BB, 0x06DC,
0x06FD, 0x071E, 0x0740, 0x0762, 0x0784, 0x07A7, 0x07CB, 0x07EF,
0x0813, 0x0838, 0x085D, 0x0883, 0x08A9, 0x08D0, 0x08F7, 0x091E,
0x0946, 0x096F, 0x0998, 0x09C1, 0x09EB, 0x0A16, 0x0A40, 0x0A6C,
0x0A98, 0x0AC4, 0x0AF1, 0x0B1E, 0x0B4C, 0x0B7A, 0x0BA9, 0x0BD8,
0x0C07, 0x0C38, 0x0C68, 0x0C99, 0x0CCB, 0x0CFD, 0x0D30, 0x0D63,
0x0D97, 0x0DCB, 0x0E00, 0x0E35, 0x0E6B, 0x0EA1, 0x0ED7, 0x0F0F,
0x0F46, 0x0F7F, 0x0FB7, 0x0FF1, 0x102A, 0x1065, 0x109F, 0x10DB,
0x1116, 0x1153, 0x118F, 0x11CD, 0x120B, 0x1249, 0x1288, 0x12C7,
0x1307, 0x1347, 0x1388, 0x13C9, 0x140B, 0x144D, 0x1490, 0x14D4,
0x1517, 0x155C, 0x15A0, 0x15E6, 0x162C, 0x1672, 0x16B9, 0x1700,
0x1747, 0x1790, 0x17D8, 0x1821, 0x186B, 0x18B5, 0x1900, 0x194B,
0x1996, 0x19E2, 0x1A2E, 0x1A7B, 0x1AC8, 0x1B16, 0x1B64, 0x1BB3,
0x1C02, 0x1C51, 0x1CA1, 0x1CF1, 0x1D42, 0x1D93, 0x1DE5, 0x1E37,
0x1E89, 0x1EDC, 0x1F2F, 0x1F82, 0x1FD6, 0x202A, 0x207F, 0x20D4,
0x2129, 0x217F, 0x21D5, 0x222C, 0x2282, 0x22DA, 0x2331, 0x2389,
0x23E1, 0x2439, 0x2492, 0x24EB, 0x2545, 0x259E, 0x25F8, 0x2653,
0x26AD, 0x2708, 0x2763, 0x27BE, 0x281A, 0x2876, 0x28D2, 0x292E,
0x298B, 0x29E7, 0x2A44, 0x2AA1, 0x2AFF, 0x2B5C, 0x2BBA, 0x2C18,
0x2C76, 0x2CD4, 0x2D33, 0x2D91, 0x2DF0, 0x2E4F, 0x2EAE, 0x2F0D,
0x2F6C, 0x2FCC, 0x302B, 0x308B, 0x30EA, 0x314A, 0x31AA, 0x3209,
0x3269, 0x32C9, 0x3329, 0x3389, 0x33E9, 0x3449, 0x34A9, 0x3509,
0x3569, 0x35C9, 0x3629, 0x3689, 0x36E8, 0x3748, 0x37A8, 0x3807,
0x3867, 0x38C6, 0x3926, 0x3985, 0x39E4, 0x3A43, 0x3AA2, 0x3B00,
0x3B5F, 0x3BBD, 0x3C1B, 0x3C79, 0x3CD7, 0x3D35, 0x3D92, 0x3DEF,
0x3E4C, 0x3EA9, 0x3F05, 0x3F62, 0x3FBD, 0x4019, 0x4074, 0x40D0,
0x412A, 0x4185, 0x41DF, 0x4239, 0x4292, 0x42EB, 0x4344, 0x439C,
0x43F4, 0x444C, 0x44A3, 0x44FA, 0x4550, 0x45A6, 0x45FC, 0x4651,
0x46A6, 0x46FA, 0x474E, 0x47A1, 0x47F4, 0x4846, 0x4898, 0x48E9,
0x493A, 0x498A, 0x49D9, 0x4A29, 0x4A77, 0x4AC5, 0x4B13, 0x4B5F,
0x4BAC, 0x4BF7, 0x4C42, 0x4C8D, 0x4CD7, 0x4D20, 0x4D68, 0x4DB0,
0x4DF7, 0x4E3E, 0x4E84, 0x4EC9, 0x4F0E, 0x4F52, 0x4F95, 0x4FD7,
0x5019, 0x505A, 0x509A, 0x50DA, 0x5118, 0x5156, 0x5194, 0x51D0,
0x520C, 0x5247, 0x5281, 0x52BA, 0x52F3, 0x532A, 0x5361, 0x5397,
0x53CC, 0x5401, 0x5434, 0x5467, 0x5499, 0x54CA, 0x54FA, 0x5529,
0x5558, 0x5585, 0x55B2, 0x55DE, 0x5609, 0x5632, 0x565B, 0x5684,
0x56AB, 0x56D1, 0x56F6, 0x571B, 0x573E, 0x5761, 0x5782, 0x57A3,
0x57C3, 0x57E2, 0x57FF, 0x581C, 0x5838, 0x5853, 0x586D, 0x5886,
0x589E, 0x58B5, 0x58CB, 0x58E0, 0x58F4, 0x5907, 0x5919, 0x592A,
0x593A, 0x5949, 0x5958, 0x5965, 0x5971, 0x597C, 0x5986, 0x598F,
0x5997, 0x599E, 0x59A4, 0x59A9, 0x59AD, 0x59B0, 0x59B2, 0x59B3 
};


