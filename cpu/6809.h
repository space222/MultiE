#pragma once
#include <functional>
#include "itypes.h"

union c6809_flags
{
	struct {
		unsigned int C : 1;
		unsigned int V : 1;
		unsigned int Z : 1;
		unsigned int N : 1;
		unsigned int I : 1;
		unsigned int H : 1;
		unsigned int F : 1;
		unsigned int E : 1;
	} b;
	u8 v;
} PACKED;

class c6809
{
public:
	// after setting these
	std::function<u8(u16)> reader = [](u16)->u8 { std::println("6809: you forgot to give a bus read function"); exit(1); return 0; };
	std::function<void(u16, u8)> writer = [](u16,u8) { std::println("6809: you forgot to give a bus write function"); exit(1); };

	// use these 
	void reset(u16 vector) { pc = vector; }
	u64 step();
	
//protected:
	u64 step10();
	u64 step11();

	u16 read16(u16 a)
	{
		u16 res = read(a)<<8;
		return res|read(a+1);
	}
	
	void write16(u16 a, u16 v)
	{
		write(a, v>>8);
		write(a+1, v);
	}
	
	void lea();
	u8 pb;
	
	u8 read(u16 a) { return reader(a); }
	void write(u16 a, u8 v) { return writer(a, v); }
	
	u8 direct(u8 a) { return read((DP<<8)|(a)); }
	void direct(u8 a, u8 v) { write((DP<<8)|(a), (v)); }

	u16 direct16(u8 a) { return (read((DP<<8)|a)<<8)|read((DP<<8)|((a+1)&0xff)); }
	void direct16(u8 a, u16 v) { write((DP<<8)|a, v>>8); write((DP<<8)|((a+1)&0xff), v); }
	
	void push8(u8 v)
	{
		write(--S, v);
	}
	
	u8 pop8()
	{
		return read(S++);
	}
	
	void push16(u16 v)
	{
		write(--S, v);
		write(--S, v>>8);	
	}
	
	u16 pop16()
	{
		u16 r = read(S++);
		r <<= 8;
		r |= read(S++);
		return r;
	}
	
	u8 add(u8, u8, u8);
	u16 add16(u16, u16, u16);
	
	u16 D() const { return (A<<8)|B; }
	void D(u16 v) { A = v>>8; B = v; }
	
	void setNZ(u8 v)
	{
		F.b.Z = ((v==0)? 1:0);
		F.b.N = ((v&0x80) ? 1:0);
	}
	
	void setNZ16(u16 v)
	{
		F.b.Z = ((v==0) ? 1:0);
		F.b.N = ((v&0x8000) ? 1:0);
	}
	
	u16 getreg(u8 reg)
	{
		switch( reg )
		{
		case 0: return D();
		case 1: return X;
		case 2: return Y;
		case 3: return U;
		case 4: return S;
		case 5: return pc;
		case 8: return 0xff00|A;
		case 9: return 0xff00|B;
		case 10: return (F.v<<8)|F.v;
		case 11: return (DP<<8)|DP;
		}
		return 0xffff;
	}	
	void setreg(u8 reg, u16 v)
	{
		switch( reg )
		{
		case 0: D(v); return;
		case 1: X = v; return;
		case 2: Y = v; return;
		case 3: U = v; return;
		case 4: S = v; return;
		case 5: pc = v; return;
		case 8: A = v; return;
		case 9: B = v; return;
		case 10: F.v = v; return;
		case 11: DP = v; return;		
		default:
			std::println("6809:setreg: invalid reg ${:X}", reg);
			break;
		}	
	}

	c6809_flags F;
	u8 A, B;
	u16 X, Y, U, S, pc, DP, ea;
	int cycles; // not a timestamp. internal use for individual instruction
};

