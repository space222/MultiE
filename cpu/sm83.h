#pragma once
#include "itypes.h"

typedef void (*sm83_writer)(u16, u8);
typedef u8 (*sm83_reader)(u16);


struct sm83
{
	u16 PC, SP, t16;
	u8 t8, IE, IF;
	union
	{
		u8 b[8];
		u16 w[4];
	} __attribute__((packed)) r;
	u64 stamp;
	
	u64 step();
	u64 prefix();
	
	u8 reg(u8);
	void set_reg(u8, u8);

	void push(u16);
	u16 pop();
	
	sm83_writer write;
	sm83_reader read;
	void reset();
	
	u16 read16(u16 addr) { u16 R = read(addr); return R|(read(addr+1)<<8); }
	bool ime, halted;
	int ime_delay;

	u8 add(u8 v1, u8 v2, u32 c);
	
	u16 add16(u16, u16);
	u8 inc(u8);
	u8 dec(u8);
	u8 rlc(u8);
	u8 rl(u8);
	u8 rrc(u8);
	u8 rr(u8);
	void daa();
	void rst(u8);
	u16 addsp(u8);
};

