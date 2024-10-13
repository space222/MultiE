#pragma once
#include "itypes.h"

typedef void (*f8_write)(u16, u8);
typedef u8 (*f8_read)(u16);

union f8flags
{
	struct {
		unsigned int S : 1;
		unsigned int C : 1;
		unsigned int Z : 1;
		unsigned int O : 1;
		unsigned int I : 1;
		unsigned int pad : 3;	
	} PACKED b;
	u8 v;
} PACKED;


class f8
{
public:
	u64 step();
	void reset();
	u8 irq_line;
	
	f8flags F;
	u8 A, isar;
	u16 pc0, pc1, dc0, dc1;
	
	u8 r[64];
	
	u8 scratch(u8);

	f8_write write, out;
	f8_read read, in;
};



