#pragma once
#include <vector>
#include "console.h"
#include "arm7tdmi.h"

class gba : public console
{
public:
	u32 fb_width() override { return 240; }
	u32 fb_height() override { return 160; }
	u32 fb_bpp() override { return 16; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	bool loadROM(const std::string) override;
	void reset() override;
	void run_frame() override;
	
	u32 read(u32, int, ARM_CYCLE);
	void write(u32, u32, int, ARM_CYCLE);
	
	arm7tdmi cpu;
	
	std::vector<u8> ROM;
	u8 bios[16*1024];
	u8 iwram[32*1024];
	u8 ewram[256*1024];
	u8 oam[1024];
	u8 palette[1024];
	u8 vram[96*1024];
	
	u16 fbuf[240*160];
};


