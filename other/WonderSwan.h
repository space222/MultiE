#pragma once
#include "console.h"
#include "8086.h"

class WonderSwan : public console
{
public:
	u32 fb_width() override { return 224; }
	u32 fb_height() override { return 144; }
	u8* framebuffer() override { return (u8*) &fbuf[0]; }
	
	void reset() override;
	void run_frame() override;
	bool loadROM(const std::string) override;
	
	void write(u32, u8);
	u8 read(u32);
	u16 in(u16, int);
	void out(u16, u16, int);
	
	void eeprom_run();
	
	c8086 cpu;
	
	u16 rom_bank0, rom_bank1, rom_linear;
	u16 sram_bank;
	u8 SYSTEM_CTRL1, SYSTEM_CTRL2;
	
	u16 eeprom_cmd, eeprom_rd, eeprom_wr;
	u8 eeprom_stat;
	
	u8 KEY_SCAN;
	
	u8 scr1_x, scr1_y;
	u8 scr2_x, scr2_y;
	u8 sprite_first, sprite_cnt;
	u8 screen_addr, sprite_table;
	
	u8 sram[64*1024];
	u8 ram[64*1024];
	std::vector<u8> ROM;
	u8 EEPROM[2048];
	u8 bios[0x2000];

	u32 fbuf[224*144];
};

