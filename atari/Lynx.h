#pragma once
#include <vector>
#include <string>
#include "console.h"
#include "6502coru.h"

class Lynx : public console
{
public:
	u32 fb_width() override { return 160; }
	u32 fb_height() override { return 102; }
	u8* framebuffer() override { return (u8*)fbuf; }
	void reset() override;
	bool loadROM(const std::string) override;
	void run_frame() override;
	
	u8 read(u16);
	void write(u16, u8);

	coru6502 cpu;
	Yieldable cycle;
	
	std::vector<u8> rom;
	u8 bios[512];
	u8 RAM[0x10000];
	u32 fbuf[160*102];
	
	void mikey_write(u16, u8);
	u8 mikey_read(u16);
	void suzy_write(u16, u8);
	u8 suzy_read(u16);
	
	void suzy_draw();
	void fb_write(int x, int y, u8 v);
	
	u8 cart_block, cart_strobe, cart_data;
	u32 cart_offset;
	u32 cart_mask, cart_shift;
	
	u8 spr[0x80];
	u8 dispctl;
	u8 palette[0x20];
	u16 fb_addr;
	
	u8 mmctrl;
	bool mmctrl_vectors() { return !(mmctrl & 8); }
	bool mmctrl_rom() { return !(mmctrl & 4); }
	bool mmctrl_mikey() { return !(mmctrl & 2); }
	bool mmctrl_suzy() { return !(mmctrl & 1); }
};

