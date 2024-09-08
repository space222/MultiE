#pragma once
#include "itypes.h"

union sh2flags
{
	struct {
		unsigned int T : 1;
		unsigned int S : 1;
		unsigned int pad : 2;
		unsigned int I : 4;
		unsigned int Q : 1;
		unsigned int M : 1;
		unsigned int pad2: 22;
	} PACKED b;
	u32 v;
} PACKED;

typedef void (*sh2write)(u32, u32, int);
typedef u32 (*sh2read)(u32, int); //, bool);

class SH2
{
public:
	u64 step();
	u64 exec(u32 addr);
	void reset();
	
	u32 read(u32, int);
	void write(u32, u32, int);
	
	sh2write memwrite;
	sh2read memread;

	u32 nextpc;
	u32 pc, pr, gbr, vbr, mach, macl;
	u32 r[16];
	sh2flags sr;
};

