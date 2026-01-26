#pragma once
#include <optional>
#include <functional>
#include "itypes.h"

union reg286
{
	struct {
		u8 l, h;
	} PACKED b ;
	u16 w;
	u32 d;
} PACKED;

union flags286
{
	struct {
		unsigned int C : 1;
		unsigned int pad1 : 1;
		unsigned int P : 1;
		unsigned int pad3 : 1;
		unsigned int A : 1;
		unsigned int pad5 : 1;
		unsigned int Z : 1;
		unsigned int S : 1;
		unsigned int TF : 1;
		unsigned int I : 1;
		unsigned int D : 1;
		unsigned int O : 1;
		unsigned int IOPL : 2;
		unsigned int NT : 1;
		unsigned int pad15 : 1;
	} PACKED b;
	u16 v;
} PACKED;

struct segdesc
{
	u32 base, limit;
	union acs_t {
		struct {
			unsigned a : 1;
			unsigned rw : 1;
			unsigned dc : 1;
			unsigned e : 1;
			unsigned s : 1;
			unsigned dpl : 2;
			unsigned p : 1;
		} PACKED b;
		u8 v;
	} PACKED access;
};

class x286
{
public:
	x286()
	{
		reset();
	}

	void step();
	void ext0f();
	
	void r8(u8 v)
	{
		if( modrm.reg >= 4 )
		{
			regs[modrm.reg&3].b.h = v;
		} else {
			regs[modrm.reg].b.l = v;
		}
	}
	
	void r16(u16 v) { regs[modrm.reg].w = v; }
	
	u8 r8() 
	{ 
		if( modrm.reg >= 4 )
		{
			return regs[modrm.reg&3].b.h;
		} else {
			return regs[modrm.reg].b.l;
		}
	}
	u16 r16()
	{
		return regs[modrm.reg].w;
	}
	
	u8 rm8()
	{
		if( modrm.mod == 3 ) { if( modrm.rm >= 4 ) return regs[modrm.rm&3].b.h; return regs[modrm.rm].b.l; }
		return read(linear, 8);
	}
	
	u16 rm16()
	{
		if( modrm.mod == 3 ) 
		{ 
			return regs[modrm.rm].w; 
		}
		
		if(  0 ) //version < 2 )
		{ // if 80186, wrap the segment
			u16 res = read((segment<<4) + offset, 8);
			res |= read((segment<<4) + ((offset+1)&0xffff), 8)<<8;
			return res;
		}
		// for 286+ at this point, the size, access, limit, etc have already been verified
		u16 res = read(linear, 8);
		res |= read(linear+1, 8)<<8;
		return res;
	}
	
	void rm8(u8 v)
	{
		if( modrm.mod == 3 ) { if( modrm.rm >= 4 ){ regs[modrm.rm&3].b.h=v; } else { regs[modrm.rm].b.l =v; } return; }
		write(linear, v, 8);
	}
	
	void rm16(u16 v)
	{
		if( modrm.mod == 3 ) { regs[modrm.rm].w = v; return; }
		
		if( 0 ) //version < 2 )
		{
			write((segment<<4)+offset, v, 8);
			write((segment<<4)+((offset+1)&0xffff), v>>8, 8);
			return;
		}
		
		write(linear, v, 8);
		write(linear+1, v>>8, 8);
	}
	
	bool validate_mem(u16 sr, segdesc& dc, u32 offs, bool isSS, bool isWrite, bool doExcept, u32 size)
	{
		return true;
		
		if( !inPMode() )
		{ // only thing that can happen in real mode is an int 13 if offset > limit (which is usually ffff)
			if( offs+size-1 > dc.limit )
			{
				if( doExcept ) general_pfault();
				return false;
			}
		
			return true;
		}
	
	
		if( (sr&~3) == 0 )
		{ // null selector GP(0)
			if( doExcept ) general_pfault();
			return false;
		}
		
		if( isWrite && (dc.access.b.e || dc.access.b.rw==0) )
		{ // write to code or read-only segment GP(0)
			if( doExcept ) general_pfault();
			return false;
		}

		if( (dc.access.b.e || dc.access.b.dc==0) && offs+size-1 > dc.limit )
		{  // code or expand-up segment read over limit SS(0) or GP(0)
			if( doExcept ) 
			{
				if( isSS )
				{
					stack_fault();
				} else {
					general_pfault();
				}
			}
			return false;
		}
		
		if( dc.access.b.e == 0 && dc.access.b.dc == 1 && offs <= dc.limit )
		{ // expand-down data segment less/eq than limit SS(0) or GP(0)
			if( doExcept )
			{
				if( isSS )
				{
					stack_fault();
				} else {
					general_pfault();
				}
			}
			return false;
		}

		return true;
	}
	
	bool validate_lea(bool isWrite, bool doExcept, u32 size)
	{
		return true;
		//return modrm.mod==3 || validate_mem(segment, desc[PDATA], offset, isStack, isWrite, doExcept, size);
	}
	
	void lea();
	
	bool ring0() { return !inPMode() || (seg[1]&3)==0; }
	bool inPMode() { return (cr[0]&1); }
	
	u8 imm8();
	u16 imm16();
	
	void general_pfault(u16 code = 0);
	void stack_fault(u16 code = 0);
	void segment_np(u16 code = 0);
	void divide_error();
	void double_fault();
	
	u8 current_exception;
	
	bool setsr(u32, u16);
	
	u8 psz8(u8 v)
	{
		F.b.S = v>>7;
		F.b.Z = v==0;
		parity8(v);
		return v;
	}
	
	u16 psz16(u16 v)
	{
		F.b.S = ((v>>15)&1);
		F.b.Z = ((v==0)?1:0);
		parity8(v);
		return v;
	}
	
	void parity8(u8);
	//void parity16(u16);
	
	std::optional<u16> pop16();
	bool push16(u16);
	
	struct {
		u32 base, limit;
	} gdtr, idtr, ldtr;
	
	u32 cr[1];
	reg286 regs[8];
	u16 seg[8];
	segdesc desc[8];
	u16 IP;
	flags286 F;
	
	u32 PDATA, REP, LOCK;
	bool halted, irq_blocked;
	u16 start_ip, start_cs;
	
	bool is_prefix(u8);
	
	struct {
		u8 orig;
		u8 mod, reg, rm;
		void set(u8 v) { orig = v; mod = v>>6; reg = (v>>3)&7; rm = v&7; }
	} modrm;
	u32 linear;
	u32 offset;
	u32 segment;
	bool isStack; // current, possibly overriden, segment is SS
	
	u32 version;
	u32 opsz;
	
	bool cond(u32);
	
	u8 add8(u8 a, u8 b, u8 c = 0);
	u16 add16(u16 a, u16 b, u16 c = 0);
	u8 shr8(u8, u32);
	u16 shr16(u16, u32);
	u8 sar8(u8, u32);
	u16 sar16(u16, u32);
	u8 shl8(u8, u32);
	u16 shl16(u16, u32);
	u8 ror8(u8, u32);
	u16 ror16(u16, u32);
	u8 rol8(u8, u32);
	u16 rol16(u16, u32);
	u8 rcr8(u8, u32);
	u16 rcr16(u16, u32);
	u8 rcl8(u8, u32);
	u16 rcl16(u16, u32);
	
	
	std::function<void(u16, u16, int)> out;
	std::function<u16(u16, int)> in;
	
	std::function<void(u32,u32,int)> write;
	std::function<u32(u32,int)> read;
	
	u8 read8(u32 addr) { return read(addr, 8); }
	u16 read16(u32 addr) { return read(addr, 16); }
	
	void far_jump(u16, u32);
	void far_call(u16, u32);
	void far_ret(u16, u32, u32);
	void interrupt(u8 n);
	
	std::function<bool(u8)> hle_int;
	
	void reset()
	{
		version = 1;
		opsz = 16;
		for(u32 i = 0; i < 8; ++i)
		{
			desc[i].limit = 0xffff;
			desc[i].base = 0;
			desc[i].access.v = 0x93;
			seg[i] = 0;
		}
		desc[1].access.v = 0x9B;
		cr[0] = 0;
		
		idtr.limit = 0x3ff;
		idtr.base = 0;
		ldtr.limit = gdtr.limit = 0;		
	}
};









