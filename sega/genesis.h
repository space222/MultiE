#pragma once
#include <string>
#include <vector>
#include "console.h"
#include "z80.h"
#include "68k.h"

class genesis : public console
{
public:
	u32 fb_width() override { return 320; }
	u32 fb_height() override { return 224; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	bool loadROM(const std::string) override;
	void reset() override;
	void run_frame() override;
	
	u32 read(u32, int);
	void write(u32, u32, int);

	z80 spu;
	m68k cpu;
	
	u64 last_target;
	u64 stamp;
	
	std::vector<u8> ROM;
	u8 RAM[0x10000];
	u8 VRAM[0x10000];
	u8 CRAM[128];
	u8 ZRAM[0x2000];
	u32 fbuf[320*224];
};

