#pragma once
#include <functional>
#include "itypes.h"

struct alignas(32) vreg
{
	u8& b(u32 index) { index &= 15; return bytes[15-index]; }
	u16& w(u32 index) { index &= 7;  return *(u16*)&bytes[((7-index)<<1)]; }
	s16& sw(u32 index) { index &= 7; return *(s16*)&bytes[((7-index)<<1)]; }
	u8 bytes[16];	
};


class n64_rsp
{
public:
	typedef void (*rsp_instr)(n64_rsp&, u32);
	rsp_instr decode(u32);
	void invalidate(u32);
	
	n64_rsp()
	{
		for(u32 i = 0; i < 8; ++i) a[i] = 0;
		for(u32 i = 0; i < 32; ++i) 
		{
			r[i] = 0;
			for(u32 e = 0; e < 8; ++e) v[i].w(e) = 0;
		}
		VCO = VCC = VCE = 0;
		IMEM = DMEM = nullptr;
		pc = npc = nnpc = 0;
	}
	vreg v[32];
	u64 a[8];
	u16 VCO, VCC, VCE;
	u16 DIV_OUT, DIV_IN;
	bool divinloaded;
	
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
		nnpc = target & 0xffc;
	}
};

