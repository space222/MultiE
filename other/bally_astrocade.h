#pragma once
#include <string>
#include "console.h"
#include "z80.h"

class bally_astrocade : public console
{
public:
	bally_astrocade();
	u32 fb_width() override { return 160; }
	u32 fb_height() override { return 102; }
	u8* framebuffer() override { return (u8*)&fbuf; }
	
	bool loadROM(const std::string) override;
	
	void run_frame() override;
	void reset() override { cpu.reset(); }
	
	u8 in(u16);
	u8 read(u16);
	void out(u16,u8);
	void write(u16,u8);

	z80 cpu;
	u8 bios[0x2000];
	u8 ROM[0x4000];
	u8 RAM[64*1024];
	u32 fbuf[320*240];
};

