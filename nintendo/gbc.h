#pragma once
#include <vector>
#include <string>
#include "console.h"
#include "sm83.h"

class gbc : public console
{
public:
	gbc() { setVsync(false); printf("gbc started\n"); }
	virtual ~gbc();
	u32 fb_width() override { return 160; }
	u32 fb_height() override { return 144; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	void reset() override;
	void run_frame() override;
	bool loadROM(const std::string) override;

	void io_write(u8, u8);
	u8 io_read(u8);
	void write(u16, u8);
	u8 read(u16);
	
	void mapper_write(u16, u8);
	u8 mbc_ram_en;
	
	u8 wram_bank, vram_bank, eram_bank;
	u16 prg_bank;
	u64 last_target;
	u8 lcd_mode;
	bool win_started;
	u8 win_line;
	void draw_scanline(u32);
	
	u16 div;
	void div_apu_tick();
	int timer_cycle_count;
	void timer_inc(u64);
	
	void apu_tick();
	void apu_write(u8,u8);
	struct {
		bool active;
		u16 phase, length;
		u32 pcnt, period;
		u8 env;
		u16 envcnt;
	} chan[4];
	u16 noise_lfsr;
	int apu_cycles_to_sample;
	
	sm83 cpu;
	u8 io[0x100];
	int mapper;
	bool isGBC, hdma_active;
	u8 cbgpal[64];
	u8 cobjpal[64];
	void draw_color_scanline(u32);
	
	std::string cartram_file;
	
	u8 bootrom[8*1024];
	std::vector<u8> ROM;
	u8 vram[16*1024];
	u8 wram[16*4096];
	u8 oam[160];
	u8 hram[0x80];
	u8 eram[128*1024];
	
	u32 fbuf[160*144];
};

