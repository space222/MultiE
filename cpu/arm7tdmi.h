#pragma once
#include "arm.h"

class arm7tdmi : public arm
{
public:
	arm7tdmi();
	void reset() { flushp(); }
	u64 step();

	void flushp() override;
	arm7_instr decode_thumb(u16);
	arm7_instr decode_arm(u32);
	
	arm7_instr thumb_funcs[0x400];
	arm7_instr arm_funcs[0x1000];

	u32 fetch, decode, exec;
	
	bool isCond(u8);
};

