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

	u32 read(u32 addr, int size);
	void write(u32 addr, u32 v, int size);


	z80 spu;
	m68k cpu;
	
	u32 fb_width() override { return 320; }
	u32 fb_height() override { return 224; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	std::vector<u8> p1;
	std::vector<u8> s1;
	std::vector<u8> m1;
	
	u16 vram[34*1024];
	u64 wram[0x10000];
	u32 fbuf[320*240];
};

