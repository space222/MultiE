#pragma once
#include "console.h"
#include "6502coru.h"

class apple2e : public console
{
public:
	apple2e() { setVsync(0); }

	u32 fb_width() override { return 280; }
	u32 fb_height() override { return 192; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	void run_frame() override;
	void reset() override;
	bool loadROM(const std::string) override;
	
	void key_down(int) override;
	//void key_up(int) override;

	u8 io_access(u16,bool);
	u8 read(coru6502&, u16);
	void write(coru6502&, u16, u8);

	coru6502 c6502;
	Yieldable cycle;
	
	u8 text_mode;
	bool hires;
	bool mixed_mode;
	
	u8 key_last, key_strobe;
	std::string paste;
	
	u8 snd_toggle;
	u32 sample_cycles;

	std::vector<u8> floppy;
	u8 bios[0x800];
	u8 ram[0x10000];
	u8 basic[0x4000];
	u8 font[8*256];
	u8 disk[256];
	void drawtile(int x, int y, int c);
	
	u32 fbuf[560*200];
};

