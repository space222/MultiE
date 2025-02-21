#pragma once
#include <vector>
#include <string>
#include "console.h"
#include "6502coru.h"

class Lynx : public console
{
public:
	u32 fb_width() override { return 160; }
	u32 fb_height() override { return 102; }
	u8* framebuffer() override { return (u8*)fbuf; }
	void reset() override;
	bool loadROM(const std::string) override;
	void run_frame() override;
	
	u8 read(u16);
	void write(u16, u8);

	coru6502 cpu;
	Yieldable cycle;
	
	std::vector<u8> rom;
	u8 bios[512];
	u8 RAM[0x10000];
	u32 fbuf[160*102];
};

