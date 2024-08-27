#pragma once
#include <vector>
#include <string>
#include "console.h"
#include "z80.h"

class pv1k : public console
{
public:
	
	u32 fb_width() override { return 240; }
	u32 fb_height() override { return 192; }
	u8* framebuffer() override { return (u8*)fbuf.data(); }
	bool loadROM(const std::string) override;
	void reset() override;
	void run_frame() override;
	
	void write(u16, u8);
	u8 read(u16);
	u8 in(u16);
	void out(u16,u8);
	
	void scanline(u32);

	u8 RAM[0x800];
	std::vector<u8> ROM;
	std::vector<u32> fbuf;
	
	u8 sq1, sq2, sq3;
	
	u8 fd_select;
	u8 snd_ctrl;
	u8 irq_en;
	u8 rom_taddr, ram_taddr;
	
	z80 cpu;
	u64 last_target, stamp;
};

