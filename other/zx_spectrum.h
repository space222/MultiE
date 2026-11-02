#pragma once
#include <print>
#include <string>
#include "console.h"
#include "z80.h"

class zx_spectrum : public console
{
public:
	u32 fb_width() override { return 256; }
	u32 fb_height() override { return 192; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	bool loadROM(const std::string) override;
	void run_frame() override;
	void reset() override;
	
	u8 read(u16);
	void write(u16, u8);
	
	u8 in(u16);
	void out(u16, u8);
	
	u64 stamp, target, last_target;

	z80 cpu;
	
	u8 ram[64*1024];
	
	u32 fbuf[256*192];
	u8 bios[16*1024];
};



