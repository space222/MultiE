#pragma once
#include <vector>
#include <string>
#include "console.h"
#include "6502coru.h"

class SuperVision : public console
{
public:
	u32 fb_width() override { return 160; }
	u32 fb_height() override { return 160; }
	u8* framebuffer() override { return (u8*)fbuf; }
	
	bool loadROM(const std::string) override;
	void reset() override;
	void run_frame() override;
	
	u8 read(u16);
	void write(u16, u8);
	
	coru6502 cpu;
	Yieldable cycle;
	
	u8 bank;
	u8 sysctrl;
	u16 timer_div;
	u8 timer;
	u8 irq_stat;
	
	std::vector<u8> ROM;
	u8 ram[0x2000];
	u8 VRAM[0x2000];
	u32 fbuf[160*160];
};

