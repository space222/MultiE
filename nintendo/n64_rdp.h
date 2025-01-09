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
		u16 to16() const { return ((b>>3)<<11)|((g>>3)<<6)|((r>>3)<<1); }
	};

	void run_commands(u64* cmd, u32 dwords);
	std::function<void()> rdp_irq;

	u8* rdram;
	u8 tmem[0x1000];

private:
	void fill_rect(u64);
	
	struct {
		u32 addr, width, bpp, format;
	} cimg, teximg;
	u32 depth_image;
	u32 fill_color;
};

