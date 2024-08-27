#pragma once
#include <vector>
#include <string>
#include "console.h"
#include "6502.h"

class a52 : public console
{
public:
	a52();
	
	void run_frame() override;
	u32 fb_width() override { return 320; }
	u32 fb_height() override { return 240; }
	u8* framebuffer() override { return (u8*)fbuf; }
	bool loadROM(std::string) override;
	void reset() override;
	
	c6502 cpu;
	
	u32 fbuf[320*240];  // 320*240 is for reusing as 5200 fbuf, prolly should just use vector here too
	std::vector<u8> RAM;
	std::vector<u8> ROM;
	std::vector<u8> BIOS;
};





