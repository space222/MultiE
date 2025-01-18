#pragma once
#include <functional>
#include "itypes.h"

class n64_rdp
{
public:

	struct dc
	{
		static dc from16(u16 c) { return { u8(((c>>11)&0x1F)<<3), u8(((c>>6)&0x1f)<<3), u8(((c>>1)&0x1f)<<3), u8((c&1)<<7) }; }
		static dc from32(u32 c) { return { u8(c>>24), u8(c>>16), u8(c>>8), u8(c) }; }
		u8 b, g, r, a;
		u32 to32() const { return (b<<24)|(g<<16)|(r<<8)|a; }
		u16 to16() const { return ((b>>3)<<11)|((g>>3)<<6)|((r>>3)<<1)|(a?1:0); }
	};

	void run_commands(u64* cmd, u32 dwords);
	std::function<void()> rdp_irq;

	u8* rdram;
	u8 tmem[0x1000];

private:
	void fill_rect(u64);
	void set_tile(u64);
	void load_tile(u64);
	void texture_rect(u64,u64);
	void texture_rect_flip(u64,u64);
	void set_tile_size(u64);
	void flat_triangle(u64,u64,u64,u64);
	void load_tlut(u64);
	void load_block(u64);
	
	u32 tex_sample(u32 tile, u32 bpp, s32 s, s32 t);
	
	struct {
		u32 addr, width, bpp, format;
	} cimg, teximg;
	
	struct {
		u32 shiftS, maskS, mirrorS, clampS;
		u32 shiftT, maskT, mirrorT, clampT;
		u32 palette;
		u32 addr, line;
		u32 bpp, format;
		s32 SL, SH;
		s32 TL, TH;
	} tiles[8];
	
	struct {
		u32 cycle_type;
		bool alpha_compare_en;
		bool force_blend;
	} other;
	
	struct {
		u32 ulX, ulY;
		u32 lrX, lrY;
		u32 odd, field;
	} scissor;

	u32 depth_image;
	u32 fill_color;
	dc blend_color, env_color, fog_color;
	u16 prim_z, prim_delta_z;
	
	const u32 CYCLE_TYPE_1CYCLE = 0;
	const u32 CYCLE_TYPE_2CYCLE = 1;
	const u32 CYCLE_TYPE_COPY = 2;
	const u32 CYCLE_TYPE_FILL = 3;
};

