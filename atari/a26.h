#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "itypes.h"
#include "console.h"
#include "6502.h"

class a26 : public console
{
public:
	a26();

	void run_frame() override;
	u32 fb_width() override { return 320; }
	u32 fb_height() override { return 228; }
	u8* framebuffer() override { return (u8*)fbuf; }
	void reset() override
	{
		cpu.reset();
	}
	bool loadROM(std::string) override;
	
	c6502 cpu;
	
	u32 fbuf[320*240];
	std::vector<u8> RAM;
	std::vector<u8> ROM;
};



