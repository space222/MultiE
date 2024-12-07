#pragma once
#include "console.h"
#include "TMS9918A.h"
#include "SN79489.h"
#include "6502.h"

class CreatiVision : public console
{
public:
	u32 fb_width() override { return vdp.fb_width(); }
	u32 fb_height() override { return vdp.fb_height(); }
	u8* framebuffer() override { return vdp.framebuffer(); }
	
	void run_frame() override;
	void reset() override;
	bool loadROM(const std::string) override;
	
	void write(u16, u8);
	u8 read(u16);
	
	std::vector<u8> ROM;
	u8 bios[2048];
	u8 ram[0x400];
	
	c6502 cpu;
	TMS9918A vdp;
	SN79489 psg;
};

