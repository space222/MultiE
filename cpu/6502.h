#pragma once
#include "itypes.h"

union c6502_flags
{
	struct {
		unsigned int C : 1;
		unsigned int Z : 1;
		unsigned int I : 1;
		unsigned int D : 1;
		unsigned int B : 1;
		unsigned int pad : 1;
		unsigned int V : 1;
		unsigned int N : 1;
	} __attribute__((packed)) b;
	u8 r;
} __attribute__((packed));

typedef void (*writer6502)(u16, u8);
typedef u8 (*reader6502)(u16);

struct c6502
{
	u8 A, X, Y, S, t8, cycle;
	u16 t16, PC, opc;
	c6502_flags P;
	bool irq_assert;
	bool nmi_edge;
	
	reader6502 read;
	writer6502 write;
	
	u64 stamp;
	
	bool allow_decimal = false;
	bool step();
	void reset();
	void nmi();
};

