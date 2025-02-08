#pragma once
#include "console.h"
#include "z80.h"

class bally_astrocade : public console
{
public:
	bally_astrocade();
	u32 fb_width() override { return 320; }
	u32 fb_height() override { return 204; }
	u8* framebuffer() override { return (u8*)&fbuf; }
	
	void run_frame() override;
	void reset() override { cpu.reset(); }
	

	z80 cpu;


};

