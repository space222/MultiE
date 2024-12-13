#pragma once
#include "console.h"
#include "6502coru.h"

class apple2e : public console
{
public:
	apple2e() : cycle() {}

	u32 fb_width() override { return 256; }
	u32 fb_height() override { return 240; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	void run_frame() override;
	void reset() override;
	bool loadROM(const std::string) override;
	
	void key_down(int) override;
	//void key_up(int) override;

	
	u8 read(coru6502&, u16);
	void write(coru6502&, u16, u8);

	coru6502 c6502;
	Yieldable cycle;
	
	u8 key_last, key_strobe;

	std::vector<u8> floppy;
	u8 bios[0x800];
	u8 ram[0x10000];
	u8 basic[0x4000];

	u32 fbuf[256*240];
};

