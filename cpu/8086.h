#pragma once
#include <concepts>
#include <bit>
#include <functional>
#include "itypes.h"

//typedef void (*c80286_writer)(u32 addr, u8 val);
//typedef u8 (*c80286_reader)(u32 addr);
//typedef void (*c80286_port_out)(u16, u16, int);
//typedef u16 (*c80286_port_in)(u16, int);

union mreg86
{
	struct
	{
		u8 l, h;
	} PACKED b;
	u16 v;
} PACKED;

union c8086_flags
{
	struct
	{
		unsigned int C : 1;
		unsigned int pad0 : 1;
		unsigned int P : 1;
		unsigned int pad1 : 1;
		unsigned int A : 1;
		unsigned int pad2 : 1;
		unsigned int Z : 1;
		unsigned int S : 1;
		unsigned int T : 1;
		unsigned int I : 1;						
		unsigned int D : 1;	
		unsigned int O : 1;
		unsigned int top4 : 4;
	} b;
	u16 v;
} PACKED;

class c8086
{
public:

struct modrm
{
	modrm(c8086* acpu, u8 am, bool do_lea = true) : cpu(*acpu), m(am)
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
	c8086& cpu;
	u8 m;
};
	
	void reset();
	u64 step();
		
	u8 read_next()
	{
		//todo: protected mode
		return read((seg[1]<<4)+(IP++));
	}
	
	u16 read_next16()
	{
		u16 res = read_next();
		res |= read_next()<<8;
		return res;
	}
	
	u16 read16(u16 s, u16 off)
	{
		u16 res = read((s<<4) + off); off++;
		res |= read((s<<4) + off)<<8;
		return res;	
	}
	
	void write16(u16 s, u16 off, u16 v)
	{
		write((s<<4)+off, v);	off++;
		write((s<<4)+off, v>>8);
	}
	
	bool is_prefix(u8);
	
	void setSZP8(std::same_as<u8> auto v)
	{
		F.b.Z = (v==0)?1:0;
		F.b.S = (v&0x80)?1:0;
		F.b.P = (std::popcount(v)&1)?0:1;
	}
	
	void setSZP16(std::same_as<u16> auto v)
	{
		F.b.Z = (v==0)?1:0;
		F.b.S = (v&0x8000)?1:0;
		F.b.P = (std::popcount(u8(v))&1)?0:1;
	}
	
	void push(u16);
	u16 pop();
	u8 add8(u8,u8,u8);
	u8 sub8(u8,u8,u8);
	u16 add16(u16,u16,u16);
	u16 sub16(u16,u16,u16);
	
	u8 seg_override(u8 index);
	
	//c80286_reader read;
	//c80286_writer write;
	//c80286_port_out port_out;
	//c80286_port_in port_in;
	std::function<u8(u32)> read;
	std::function<void(u32,u8)> write;
	std::function<void(u16,u16,int)> port_out;
	std::function<u16(u16,int)> port_in;
	
	struct {
		u8 irr, isr, imr;
		u8 icw[4];
		u8 ocw[3];
		u8 read_reg;
		u8 state;
	} pic;
	
	bool halted;
	u32 prefix;
	u16 start_ip, start_cs;
	u16 IP;
	u16 seg[6];
	mreg86 r[8];
	c8086_flags F;
	bool irq_blocked;
};

