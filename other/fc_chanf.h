#pragma once
#include <vector>
#include "console.h"
#include "f8.h"

class fc_chanf : public console
{
public:
	u32 fb_width() override { return 128; }
	u32 fb_height() override { return 64; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	bool loadROM(const std::string) override;
	void reset() override;
	void run_frame() override;
	
	void write(u16,u8);
	u8 read(u16);
	void out(u16,u8);
	u8 in(u16);
	
	u64 stamp, last_target;
	u8 port[6];
	void plot_pixel();

	f8 cpu;
	std::vector<u8> rom;
	u8 ram[64*1024];
	u8 vram[128*64];
	u8 bios[8*1024];
	u32 fbuf[128*64];
};

