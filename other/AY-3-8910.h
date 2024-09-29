#pragma once
#include "itypes.h"

class AY_3_8910
{
public:
	u8 cycles(u64);
	void clock();
	void reset() { stamp = 0; }
	
	void set(u8 i) { index = i&0xf; }
	u8 read() { return regs[index]; }
	void write(u8 v) { regs[index] = v; }

	u64 stamp;
	u8 index;
	u8 out;
	u8 regs[16];
};

