#pragma once
#include <vector>
#include <string>
#include "console.h"
#include "68k.h"

class jaguar : public console
{
public:
	u32 fb_width() { return 720; }
	u32 fb_height() { return 448; }
	u8* framebuffer() { return (u8*)&fbuf[0]; }
	
	bool loadROM(const std::string) override;
	void reset() override;
	void run_frame() override;
	
	void write(u32, u32, int);
	u32 read(u32, int);
	
	m68k cpu;

	std::vector<u32> fbuf;
	std::vector<u8> ROM;
	u8 RAM[2*1024*1024];
	u8 bootrom[0x20000];
	u8 gram[0x1000];
	bool memcon_written;
	u16 MEMCON1, MEMCON2;
};




