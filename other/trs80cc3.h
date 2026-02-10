#pragma once
#include <print>
#include "util.h"
#include "6809.h"
#include "console.h"

class trs80cc3 : public console
{
public:
	~trs80cc3()
	{
		FILE* fp =fopen("ramdump.bin", "wb");
		fwrite(ram, 1, 0x78000, fp);
		fclose(fp);
	}

	u32 fb_width() override { return 256; }
	u32 fb_height() override { return 192; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }

	void run_frame() override;
	void reset() override;
	bool loadROM(std::string) override;
	
	u8 read_io(u8 a);
	void write_io(u8 a, u8 v);

	void write(u16 a, u8 v);
	u8 read(u16 a);
	
	u8 keyboard_state();

	bool rom_enabled = true;
	
	u64 stamp=0;
	u64 last_target=0;

	c6809 cpu;
	u8 ram[512_KB];
	u8 bios[32_KB];
	u8 io[0x100];
	u32 fbuf[640*240];
};






