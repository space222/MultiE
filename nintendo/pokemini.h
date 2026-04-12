#pragma once
#include <print>
#include <vector>
#include "console.h"
#include "s1c88.h"

class pokemini : public console
{
public:
	u32 fb_width() override { return 96; }
	u32 fb_height() override { return 64; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	void run_frame() override;
	void reset() override;
	bool loadROM(std::string) override;

	s1c88 cpu;

	u8 read(u32);
	void write(u32, u8);
	
	u8 io_read(u32);
	void io_write(u32, u8);

	u8 RAM[4_KB];
	u8 bios[4_KB];
	std::vector<u8> rom;
	
	u32 fbuf[97*64];
};



