#pragma once
#include "itypes.h"
#include <cstring>
#include <vector>

class TMS9918A
{
public:
	TMS9918A() : fbuf(256*192) { reset(); }
	u32 fb_width() { return (vdp_regs[1]&0x10) ? 240 : 256; }
	u32 fb_height() { return 192; }
	void draw_scanline(u32 line);
	u8* framebuffer() { return (u8*)fbuf.data(); }
	
	void ctrl(u8 v);
	void data(u8 v);
	u8 read() { return 0; } // todo
	u8 stat() { return 0; } // todo
	
	void reset() { memset(vdp_regs, 0, 8); vdp_ctrl_latch = false; }
	
	std::vector<u32> fbuf;
	u8 vdp_cd, rdbuf;
	u16 vdp_addr;
	u8 vdp_regs[8];
	u8 vram[0x4000];
	u8 vdp_ctrl_stat;
	bool vdp_ctrl_latch;
};



