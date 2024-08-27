#pragma once
#include "80286.h"

struct modrm
{
	modrm(c80286* acpu, u8 am, bool do_lea = true) : cpu(*acpu), m(am)
	{
		if( do_lea )
		{
			lea();
		}
	}
	
	u8 reg8() { return (m&0x20) ? cpu.r[(m>>3)&3].b.h : cpu.r[(m>>3)&3].b.l; }
	void reg8(u8 v) { if( m&0x20 ) cpu.r[(m>>3)&3].b.h = v; else cpu.r[(m>>3)&3].b.l = v; }

	u32 reg16() { return cpu.r[(m>>3)&7].v; }
	void reg16(u32 v) { cpu.r[(m>>3)&7].v = v; }
	
	u8 rm8()
	{
		if( (m&0xc0) == 0xc0 ) { if( m&4 ) return cpu.r[m&3].b.h; return cpu.r[m&3].b.l; }
		return cpu.read((ea_seg<<4) + ea_off);	
	}
	
	void rm8(u8 v)
	{
		if( (m&0xc0) == 0xc0 )
		{
			if( m&4 ) cpu.r[m&3].b.h = v;
			else cpu.r[m&3].b.l = v;
			return;
		}
		cpu.write((ea_seg<<4)+ea_off, v);
	}
	
	u16 rm16()
	{
		if( (m&0xc0) == 0xc0 ) return cpu.r[m&7].v;
		return cpu.read16(ea_seg, ea_off);	
	}
	
	void rm16(u16 v)
	{
		if( (m&0xc0) == 0xc0 )
		{
			cpu.r[m&7].v = v;
			return;
		}
		cpu.write16(ea_seg, ea_off, v);	
	}

	void lea()
	{
		if( (m&0xc0) == 0xc0 ) return;
		if( (m&0xc7) == 6 )
		{
			segreg_ind = cpu.seg_override(3);
			ea_seg = cpu.seg[segreg_ind];
			ea_off = cpu.read_next16();
			return;
		}
		
		u16 disp16 = 0;
		if( (m&0xc0) == 0x40 ) disp16 = (s16)(s8)cpu.read_next();
		else if( (m&0xc0) == 0x80 ) disp16 = cpu.read_next16();
		segreg_ind = 3;
		
		switch( m & 7 )
		{
		case 0: ea_off = cpu.r[3].v + cpu.r[6].v + disp16; break;
		case 1: ea_off = cpu.r[3].v + cpu.r[7].v + disp16; break;
		case 2: ea_off = cpu.r[5].v + cpu.r[6].v + disp16; segreg_ind = 2; break;
		case 3: ea_off = cpu.r[5].v + cpu.r[7].v + disp16; segreg_ind = 2; break;
		case 4: ea_off = cpu.r[6].v + disp16; break;
		case 5: ea_off = cpu.r[7].v + disp16; break;
		case 6: ea_off = cpu.r[5].v + disp16; segreg_ind = 2; break;
		case 7: ea_off = cpu.r[3].v + disp16; break;
		}
		
		segreg_ind = cpu.seg_override(segreg_ind);
		ea_seg = cpu.seg[segreg_ind];
	}
	
	u8 segreg_ind;
	u16 ea_seg, ea_off;
	c80286& cpu;
	u8 m;
};

