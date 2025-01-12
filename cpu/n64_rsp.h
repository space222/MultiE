#pragma once
#include <functional>
#include "itypes.h"

struct vreg
{
	u8& b(u32 index) { index &= 15; return bytes[15-index]; }
	u16& w(u32 index) { index &= 7;  return *(u16*)&bytes[((7-index)<<1)]; }
	s16& sw(u32 index) { index &= 7; return *(s16*)&bytes[((7-index)<<1)]; }
	u8 bytes[16];	
};


class n64_rsp
{
public:
	u64 a[8];
	vreg v[32];
	u16 VCO, VCC, VCE;
	
	u32 r[32];
	u32 pc, npc, nnpc;
	void step();
	u8* IMEM;
	u8* DMEM;
	std::function<void()> broke;
	std::function<void(u32, u32)> sp_write;
	std::function<u32(u32)> sp_read;
	std::function<void(u32, u32)> dp_write;
	std::function<u32(u32)> dp_read;
	
	void branch(u32 target)
	{
		nnpc = target;
	}
};

