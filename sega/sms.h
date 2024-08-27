#pragma once
#include <vector>
#include <deque>
#include <string>
#include "console.h"
#include "z80.h"
#include "SN79489.h"

class SMS : public console
{
public:
	SMS() { setVsync(0); }
	void run_frame() override;
	u32 fb_width() override { return 256; }
	u32 fb_height() override { return ( (vdp_regs[0]&BIT(2)) && (vdp_regs[1]&BIT(4)) ) ? 224 : 192; }
	u8* framebuffer() override { return (u8*)fbuf; }
	void reset() override
	{
		vdp_ctrl_latch = false;
		cpu.sp = 0xdff0;
		cpu.reset();
		bankreg[1] = 0;
		bankreg[2] = 1;
		bankreg[3] = 2;
		for(int i = 0; i < 16; ++i) vdp_regs[i] = 0;
		stamp = snd_cycles = last_target = 0;
		PSG.reset();
		samples.clear();
		samples.reserve(735);
	}
	bool loadROM(std::string) override;
	
	void write8(u16, u8);
	u8 read8(u16);
	void out8(u16, u8);
	u8 in8(u16);
	
	void vdp_ctrl(u8);
	void vdp_data(u8);
	
	u8 vdp_cd, vdp_rdbuf;
	u16 vdp_addr;
	u8 vdp_regs[16];
	u8 sprite_buf[16];
	u8 vcount;
	u8 vdp_ctrl_stat;
	u8 rdbuf;
	u32 num_sprites;
	bool vdp_ctrl_latch;
	void set_vcount(u32);
	void vdp_scanline(u32);
	void eval_sprites(u32);
	
	u8 bankreg[4];
	
	u64 stamp;
	u64 last_target;
	z80 cpu;
	SN79489 PSG;
	
	u64 snd_cycles;
	std::vector<float> samples;
	
	u32 fbuf[256*240];
	u8 vram[0x4000];
	u8 cram[0x40];
	std::vector<u8> RAM;
	std::vector<u8> ROM;
};


