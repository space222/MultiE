#pragma once
#include <vector>
#include <string>
#include "console.h"
#include "cp1600.h"

class intellivision : public console
{
public:
	intellivision();
	u32 fb_width() override { return 159; }
	u32 fb_height() override { return 192; }
	u8* framebuffer() override { return (u8*)fbuf.data(); }
	
	void run_frame() override;
	void reset() override;
	bool loadROM(std::string) override;
	
	u8 read_left_cont();
	
	void write(u16,u16);
	u16 read(u16);
	
	cp1600 cpu;

	bool stic_strobe;
	u8 stic_mode;
	u8 stic_stack;
	u8 stic_regs[0x30];
	bool in_vblank;

	std::vector<u32> fbuf;
	std::vector<u16> ROM;
	u16 sysram[0x200];
	u8 scratch[0x200];
	u16 exec[0x1000];
	u8 grom[0x800];
	u8 gram[512];
};

