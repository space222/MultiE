#pragma once
#include <functional>
#include "itypes.h"

class n64_rsp
{
public:
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

