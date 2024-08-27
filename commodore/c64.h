#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "console.h"
#include "6502.h"
#include "TAPFile.h"

class c64 : public console
{
public:
	c64();
	
	void run_frame() override;
	bool loadROM(std::string) override;
	u32 fb_width() override { return 320; }
	u32 fb_height() override { return 240; }
	u8* framebuffer() override { return (u8*)fbuf.data(); }
	void reset() override
	{
		cpu.reset();
	}
	
	c6502 cpu;
	
	TAPFile* tape;
	
	std::vector<u32> fbuf;
	std::vector<u8> RAM;
	std::vector<u8> BIOS;
	std::vector<u8> BASIC;
	std::vector<u8> CHARSET;
};


