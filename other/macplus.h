#pragma once
#include "console.h"
#include "68k.h"

class macplus : public console
{
public:
	void reset() override;
	void run_frame() override;
	bool loadROM(const std::string) override;
	u32 fb_width() override { return 512; }
	u32 fb_height() override { return 342; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }

	void write(u32 addr, u32 v, int size);
	u32 read(u32 addr, int size);
	
	m68k cpu;
	u64 stamp, last_target;
	
	u32 fbuf[512*342];

	u8 ram[4*1024*1024];
	u8 rom[512*1024];
	
	u8 via_a, via_b, via_ie, via_if, via_pc;
};



