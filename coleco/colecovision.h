#pragma once
#include <functional>
#include <vector>
#include "console.h"
#include "z80.h"
#include "SN79489.h"
#include "TMS9918A.h"

class colecovision : public console
{
public:
	colecovision() { setVsync(0); }
	u32 fb_scale_w() override { return 256; }
	u32 fb_scale_h() override { return 192; }
	u32 fb_width() override { return 256; }
	u32 fb_height() override { return 192; }
	void reset() override;
	void run_frame() override;
	bool loadROM(const std::string) override;
	u8* framebuffer() override { return vdp.framebuffer(); }
	
	u8 cv_read(u16);
	void cv_write(u16, u8);
	u8 cv_in(u16);
	void cv_out(u16, u8);
	
	u32 sample_cycles;
	u64 stamp, last_target;
	
	TMS9918A vdp;
	
	z80 cpu;
	SN79489 psg;
	u8 bios[0x2000];
	u8 ram[0x400];
	u8 rom[0x8000];
};

