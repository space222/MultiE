#pragma once
#include <vector>
#include "console.h"
#include "68k.h"
#include "z80.h"

class AES : public console
{
public:
	void run_frame() override;
	void reset() override;
	bool loadROM(const std::string) override;
	bool loadNEO(const std::string);
	
	u32 fb_width() override { return 320; }
	u32 fb_height() override { return 224; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	u32 read(u32 addr, int size);
	void write(u32 addr, u32 v, int size);
	
	void draw_scanline(int line);

	bool bios_vectors, bios_grom;
	
	u8 bios[0x20000];
	u8 palette[0x4000];
	bool vblank_irq_active, timer_irq_active, irq3_active;
	u16 REG_LSPCMODE, REG_VRAMADDR;
	s16 REG_VRAMMOD;
	u32 palbank;
	u32 line_counter = 0;
	
	
	u64 mstamp;
	u64 last_target;
	
	u8 rtc_val;
	u8 z80_cmd, z80_reply;
	
	z80 spu;
	m68k cpu;
	std::vector<u8> p1;
	u32 pbank;
	std::vector<u8> s1;
	std::vector<u8> m1;
	
	u8 backup[0x10000];
	u16 vram[34*1024];
	u8 wram[0x10000];
	u32 fbuf[320*240];
};

