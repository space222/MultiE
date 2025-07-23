#pragma once
#include "itypes.h"

class LCDEngine
{
public:
	LCDEngine(u8* v, u8* p, u8* o) : VRAM(v), palette(p), oam(o) {}
	
	void render_sprites(int);
	void draw_scanline(u32);
	void draw_mode_0(u32);
	void draw_mode_1(u32);
	void draw_mode_2(u32);
	void draw_mode_3(u32);
	void draw_mode_4(u32);
	void draw_mode_5(u32);
	
	u8* VRAM;
	u8* palette;
	u8* oam;
	u16 regs[48];
	u32 fbuf[256*192];
	u8 objwin[256];
	u8 spr[256];
	u8 spr_pri[256];
	u8 bg[4];
};

