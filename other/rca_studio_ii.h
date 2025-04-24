#pragma once
#include "console.h"
#include "rca1802.h"

class rca_studio_ii : public console
{
public:
	u32 fb_width() override { return 64; }
	u32 fb_height() override { return 128; }
	u8* framebuffer() override { return (u8*)fbuf; }
	void reset() override;
	void run_frame() override;

	bool loadROM(const std::string) override;

	rca1802 cpu;
	u8 ram[512];
	u8 bios[2*1024];
	std::vector<u8> ROM;
	
	u32 fbuf[128*64];
};

