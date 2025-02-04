#pragma once
#include <functional>
#include <vector>
#include "itypes.h"

class n64_rdp
{
public:

	struct dc
	{
		dc() {}
		dc(s32 R, s32 G, s32 B, s32 A)
		{
			if( R < 0 ) R = 0; else if( R > 255 ) R = 255;
			if( G < 0 ) G = 0; else if( G > 255 ) G = 255;
			if( B < 0 ) B = 0; else if( B > 255 ) B = 255;
			if( A < 0 ) A = 0; else if( A > 255 ) A = 255;
			r = R; g = G; b = B; a = A;
		}
		static dc from16(u16 c) { return { u8(((c>>11)&0x1F)<<3), u8(((c>>6)&0x1f)<<3), u8(((c>>1)&0x1f)<<3), u8((c&1)?0xff:0) }; }
		static dc from32(u32 c) { return { u8(c>>24), u8(c>>16), u8(c>>8), u8(c) }; }
		u8 b, g, r, a;
		u32 to32() const { return (r<<24)|(g<<16)|(b<<8)|a; }
		u16 to16() const { return ((r>>3)<<11)|((g>>3)<<6)|((b>>3)<<1)|(a?1:0); }
		
		dc operator*=(const dc& p)
		{
			b = ((b/255.f) * (p.b/255.f))*255;
			g = ((g/255.f) * (p.g/255.f))*255;
			r = ((r/255.f) * (p.r/255.f))*255;
			a = ((a/255.f) * (p.a/255.f))*255;
			return *this;
		}
	};

	void recv(u64);
	std::function<void()> rdp_irq;

	u8* rdram;
	u8 b9[0x800000];
	u8 tmem[0x1000];

private:
	std::vector<u64> cmdbuf;
	void fill_rect(u64);
	void set_tile(u64);
	void load_tile(u64);
	void texture_rect(u64,u64);
	void texture_rect_flip(u64,u64);
	void set_tile_size(u64);
	void load_tlut(u64);
	void load_block(u64);
	void triangle();
	
	dc tex_sample(u32 tile, s32 s, s32 t);
	
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
		bool z_compare, z_write;
	} other;
	
	struct {
		u32 ulX, ulY;
		u32 lrX, lrY;
		u32 odd, field;
	} scissor;
	
	struct {
		s64 y1, y2, y3;
		s64 xh, xm, xl;
		s64 r, g, b, a;
		s64 s, t, w;
		s64 z;
		
		s64 DxhDy, DxmDy, DxlDy;
	
		s64 DrDx, DgDx, DbDx, DaDx;
		s64 DrDe, DgDe, DbDe, DaDe;
		s64 DrDy, DgDy, DbDy, DaDy;

		s64 DsDx, DtDx, DwDx;
		s64 DsDe, DtDe, DwDe;
		s64 DsDy, DtDy, DwDy;

		s64 DzDx, DzDe, DzDy;
		
		u8 cmd;
	} RS;
	
	struct {
		u32 tile;	
	} TX;

	u32 depth_image;
	u32 fill_color;
	dc blend_color, env_color, fog_color;
	u16 prim_z, prim_delta_z;
	
	const u32 CYCLE_TYPE_1CYCLE = 0;
	const u32 CYCLE_TYPE_2CYCLE = 1;
	const u32 CYCLE_TYPE_COPY = 2;
	const u32 CYCLE_TYPE_FILL = 3;
};

