#pragma once
#include <vector>
#include <print>
#include "console.h"
#include "HuC6280.h"

class tg16 : public console
{
public:
	u32 fb_width() override { return 256; }
	u32 fb_height() override { return 240; }
	u32 fb_bpp() override { return 16; }
	u8* framebuffer() override{ return (u8*)&fbuf[0]; }
	void reset() override;
	void run_frame() override;
	bool loadROM(std::string) override;

	u8 read(u32 addr);
	void write(u32 addr, u8 v);
	u8 io_read(u32 addr);
	void io_write(u32 addr, u8 v);
	
	struct {
		u8 latch;
		u16 memaddr, rdbuf;
		u16 regs[0x20];
		u16 palram[512];
		u16 paddr;
		
		u16 sat[4*64];
		
		union stat_t
		{
			struct {
				bitfield cr : 1;
				bitfield ovr : 1;
				bitfield rr : 1;
				bitfield ds : 1;
				bitfield dv : 1;
				bitfield vd : 1;
				bitfield bsy : 1;
				bitfield unu : 1;			
			} PACKED b;
			u8 v;
		} PACKED stat;
		
		u8 bg[256];
	} vdc;
	void vdc_sat_dma();
	void system_cycles(u64);
	void draw_line(u32 scanline, u32 bgline);
	
	u64 stamp, last_target;

	HuC6280 cpu;

	std::vector<u8> ROM;
	u8 wram[8_KB];
	u8 VRAM[64_KB];

	u16 fbuf[320*256];
};

