#pragma once
#include <cstdio>
#include <cstdlib>
#include "itypes.h"

typedef void (*c65816_write)(u32, u8);
typedef u8 (*c65816_read)(u32);

class c65816
{
public:
	union reg
	{
		struct __attribute__((packed)) 
		{
			u8 l, h;
		} b;
		u16 v;
	} __attribute__((packed));

	void reset();
	u64 step();

	u16 PC, DP, SP;
	reg C, X, Y, B;
	u8  PB, DB;
	u8  emu_mode;
};



