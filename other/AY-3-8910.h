#pragma once
#include <cstdio>
#include "itypes.h"

class AY_3_8910
{
public:
	u8 cycles(u64);
	void clock();
	void reset() 
	{ 
		stamp = 0; 
		cnt[0] = cnt[1] = cnt[2] = 0; 
		p[0] = p[1] = p[2] = 0;
		regs[7] = 0xff;
		regs[0] = regs[2] = regs[4] = 0xff;
	}
	
	void set(u8 i) { index = i&0xf; }
	u8 read() { return regs[index]; }
	void write(u8 v) 
	{ 
		regs[index] = v; 
	}

	u16 cnt[3];
	u8 p[3];

	u64 stamp;
	u8 index;
	u8 out;
	u8 regs[16];
};

