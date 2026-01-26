#pragma once
#include <functional>
#include <print>
#include "itypes.h"

class EEngine
{
public:

	void step();
	
	void exception(u32);
	
	std::function<u128(u32, int)> read;
	std::function<void(u32, u128, int)> write;
	
	u64 stamp;

	bool ndelay, delay;
	u32 pc, npc, nnpc;
	u64 hi1, hi, lo1, lo;
	u32 sa;
	
	void branch(u32 addr)
	{
		ndelay = true;
		nnpc = addr;
	}
	
	void write_cop0(u32 reg, u32 v) { cop0[reg] = v; }
	u32 read_cop0(u32 reg) { return cop0[reg]; }

	union EEReg
	{
		u64 d;
		s64 sd;
		
		u64 ud[2];
		
		u128 q;
		
		u32 w[4];
		u16 h[8];
		s32 sw[4];
		s16 sh[8];
		EEReg& operator=(u64 v)
		{
			d = v;
			return *this;
		}
		
		operator u64()
		{
			return d;
		}
	
	} PACKED r[32];

	u32 cop0[32];
	float fpr[32];


};

