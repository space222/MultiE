#pragma once
#include <vector>
#include "console.h"
#include "6502.h"

class nes;

typedef void (*nes_mapper_write)(nes*, u16, u8);
typedef void (*nes_mapper_ppu_bus)(nes*, u16);

class nes : public console
{
public:
	nes();
	u8* framebuffer() override { return (u8*) fbuf.data(); }
	u32 fb_width() override { return 256; }
	u32 fb_height() override { return 240; }
	void reset() override;
	
	void run_frame() override;
	bool loadROM(std::string) override;
	
	void write(u16 addr, u8 val);
	u8 read(u16 addr);
	
	void ppu_write(u8 addr, u8 val);
	u8 ppu_read(u8 addr);
	u8 ppu_bus(u16 addr);
	void ppu_bus(u16 addr, u8 val);
	void ppu_dot();
	void draw_scanline();
	u16 loopy_v, loopy_t;
	u8 scroll_x, scroll_y;
	u16 fine_x, fine_y;
	int ppu_latch;
	u8 ppu_regs[8];
	int dot, scanline;
	bool frame_complete;
	u8 ntram[0x1000];
	u8* nametable[4];
	u8 ppu_palmem[0x20];
	
	u8 pad1_index, pad2_index;
	u8 pad_strobe;
	
	nes_mapper_write mapper_write;
	nes_mapper_ppu_bus mapper_ppu_bus;
	int mapper;
	
	void apu_write(u16,u8);
	void apu_clock();
	void apu_clock_env_lincnt();
	void apu_clock_len_sweep();
	void apu_reset();
	u32 apu_stamp;
	u8 apu_4017, apu_regs[24];
	u8 pulse1_env_div, pulse1_env_vol, p1_env_start;
	u8 pulse2_env_div, pulse2_env_vol, p2_env_start;
	u8 noise_env_div, noise_env_vol, noise_env_start;
	u8 tri_lin_reload;
	u8 pulse1_seq, pulse2_seq, tri_seq, noise_seq;
	u16 pulse1_len, pulse2_len, tri_len, tri_lin, noise_len;
	u16 pulse1_timer, pulse2_timer, tri_timer, noise_timer;
	u8 pulse1_vol, pulse2_vol;
	u16 noise_shift;
	u16 dmc_timer, dmc_cur_addr, dmc_len;
	u8 dmc_byte, dmc_bindex;
	u8 p1_sweep_reload, p2_sweep_reload;
	u8 p1_sweep_div, p2_sweep_div;
	
	bool chrram;
	u8 header[16];
	
	u8* chrbanks[8];
	u8* prgbanks[4];
		
	c6502 cpu;
	std::vector<u32> fbuf;
	std::vector<u8> PRG;
	std::vector<u8> CHR;
	std::vector<u8> SRAM;
	u8 OAM[0x100];
	u8 RAM[0x800];
	
	bool isFDS;
	u8 fds_bios[8*1024];
	u32 fds_sides;
	std::vector<u8> floppy;
	void fds_reg_write(u16, u8);
	u8 fds_reg_read(u16);
};



