#pragma once
#include <vector>
#include "console.h"
#include "sm83.h"

class dmg : public console
{
public:
	dmg();
	
	void run_frame() override;
	
	bool loadROM(std::string) override;
	void reset() override;

	u32 fb_width() override { return 160; }
	u32 fb_height() override { return 144; }
	u8* framebuffer() override { return (u8*)fbuf.data(); }
	sm83 cpu;
	
	int mapper, ram_type;
	u32 rombank, rambank;
	u32 num_rombanks, num_rambanks;
	bool bootrom_active;
	bool rtc_regs_active;
	u16 DIV;
	
	void scanline(int);

	std::vector<u32> fbuf;
	std::vector<u8> ROM;
	std::vector<u8> RAM;
	std::vector<u8> VRAM;
	std::vector<u8> SRAM;
	u8 boot[0x100];
	u8 IO[0x100];
	u8 OAM[0xA0];
};

enum mapper_id { NO_MBC, MBC1, MBC2, MBC3, MBC5, MBC6, MBC7 };
enum exram_type { NO_RAM, KB2, BANKS1, BANKS4, BANKS16, BANKS8, MBC2_RAM }; 


