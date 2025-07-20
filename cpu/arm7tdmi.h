#pragma once
#include "arm.h"

typedef void (*arm7_instr)(arm&, u32);



class arm7tdmi : public arm
{
public:
	void reset();
	void step();

	arm7_instr decode_thumb(u16);
	arm7_instr decode_arm(u32);
	
	arm7_instr thumb_funcs[0x400];
	arm7_instr arm_funcs[0x1000];

	u32 fetch, decode, execute;
	
	//void do_fetch();
	
	void switch_to_mode(u32) override;
	void flushp() override;
	void swi() override;
	bool isCond(u8) override;
};

