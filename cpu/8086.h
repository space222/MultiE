#pragma once
#include "itypes.h"

typedef void (*x8086_writer)(u32, u8);
typedef u8 (*x8086_reader)(u32);
typedef void (*x8086_port_out)(u16, u16, int);
typedef u16 (*x8086_port_in)(u16, int);

union mreg
{
	u16 v;
	struct
	{
		u8 l, h;
	} b;
} __attribute__((packed));

union flags86
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
		unsigned int top4 : 1;
	} b  __attribute__((packed));	
	u8 v;
} __attribute__((packed));

struct x8086
{
	u64 step();
	u64 exec(u8);
	void reset();
	u64 stamp;
	
	x8086_reader read;
	x8086_writer write;
	x8086_port_out port_out;
	x8086_port_in port_in;
	
	u16 read16(u16 sr, u16 off) 
	{ 
		u16 res = read((sr<<4)+off);
		off++;
		res |= read((sr<<4)+off)<<8;
		return res;
	}
	
	void write16(u16 sr, u16 off, u16 val)
	{
		write((sr<<4)+off, val);
		off++;
		write((sr<<4)+off, val>>8);
	}
	
	u8 add8(u8,u8,u8);
	u16 add16(u16,u16,u16);
	void push16(u16);
	u16 pop16();
	
	void setSZP8(u8);
	void setSZP16(u16);
	
	u16 seg_override(u16);
	
	u16 start_ip, start_cs;
	
	struct {
		u8 irr, isr, imr;
		u8 icw[4];
		u8 ocw[3];
		u8 read_reg;
		u8 state;
	} pic;
	
	u8 prefix;
	bool is_prefix(u8);
	bool irq_assert, irq_blocked;
	u16 pc;
	flags86 F;
	mreg r[8];
	u16 seg[4];
};

