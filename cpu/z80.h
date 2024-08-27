#pragma once
#include <bit>
#include "itypes.h"

typedef void (*z80_write8)(u16, u8);
typedef u8 (*z80_read8)(u16);

union z80_flags
{
	struct {
		unsigned int fC : 1;
		unsigned int N : 1;
		unsigned int V : 1;
		unsigned int F3 : 1;
		unsigned int fH : 1;
		unsigned int F5 : 1;
		unsigned int Z : 1;
		unsigned int S : 1;
	} __attribute__((packed)) b;
	u8 v;
} __attribute__((packed));

struct z80
{
	u8 r[24];
	z80_flags fl;
	u16 pc, sp;
	u8 irq_line, iff1, iff2, R, Ibase;
	bool halted, intr_blocked;
	u8 prefix;
	u64 icycles;
	void reset();
	u64 step();
	u64 step_cb();
	u64 step_ed();
	
	u8& B() { return r[0]; }
	u8& C() { return r[1]; }
	u8& D() { return r[2]; }
	u8& E() { return r[3]; }
	u8& H() { return r[4]; }
	u8& L() { return r[5]; }
	u8& F() { return fl.v; }
	u8& A() { return r[7]; }
	u8& IXL() { return r[8]; }
	u8& IXH() { return r[9]; }
	u8& IYL() { return r[10]; }
	u8& IYH() { return r[11]; }
	
	z80_write8 write, out;
	z80_read8 read, in;
	
	u16 read16(u16 addr)
	{
		u16 temp = read(addr);
		temp |= (read(addr+1)<<8);
		return temp;
	}
	
	void set35(u8 v)
	{
		fl.b.F3=fl.b.F5=0;
		fl.v |= (v&0x28);
	}
	
	void setSZP(u8 v)
	{
		fl.b.S = v>>7;
		fl.b.Z = (v==0 ? 1 : 0);
		fl.b.V = (std::popcount(v)&1)^1;
	}
	
	u16 add16(u16, u16, u16 c = 0);
	u16 add16f(u16,u16,u16);
	u8 adc(u8, u8, u8);
	
	u8 inc(u8);
	u8 dec(u8);
	
	void rcla();
	void rrca();
	void rra();
	void rla();
	void daa();
	
	u16 pop()
	{
		u16 temp = read(sp++);
		temp |= (read(sp++)<<8);
		return temp;
	}
	
	void push(u16 v)
	{
		write(--sp, v>>8);
		write(--sp, v);
	}
	
	void write16(u16 a, u16 v)
	{
		write(a, v);
		write(a+1, v>>8);
	}
	
	void ret()
	{
		pc = pop();
	}
	
	void call(u16 a)
	{
		push(pc);
		pc = a;
	}
	
	u8 getreg(u8);
	void setreg(u8,u8);
	u16 getHL();
	void setHL(u16);
	
	void setBC(u16);	
	void setDE(u16);
	
	void setAF(u16 v)
	{
		A() = v>>8;
		fl.v = v;
	}
	
	u32 cycles_til_sample;
	u16 keys;
};

