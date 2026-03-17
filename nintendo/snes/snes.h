#pragma once
#include <print>
#include <vector>
#include "console.h"
#include "65816.h"
//#include "spc700.h"

class snes : public console
{
public:
	u32 fb_bpp() override { return 16; } // bgr555
	u32 fb_width() override { return 512; }
	u32 fb_height() override { return 240; }
	u32 fb_scale_h() override { return 480; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	void reset() override { cpu.E=1; cpu.pc = cpu.bus_read(0xfffc); cpu.pc |= cpu.bus_read(0xfffd)<<8; cpu.opc = cpu.bus_read(cpu.pc); cpu.pb=0; cpu_run = cpu.run(); }

	void run_frame() override;
	bool loadROM(std::string) override;
	
	u8 read(u32);
	void write(u32, u8);
	
	struct {
		s64 master_cycles=0;
		bool frame_complete=false;
		u32 scanline=0;
	} ppu;
	void ppu_mc(s64);
	void ppu_draw_scanline();
	
	enum cartmapping { MAPPING_LOROM=0, MAPPING_HIROM=1, MAPPING_EXHIROM=5 };
	struct {
		u32 mapping_type=0;
		u8 chipset=0;
		bool fastROM=false;
		u32 rom_size;
		u32 ram_size;
	} cart;

	u16 fbuf[512*240];

	c65816 cpu;
	Yieldable cpu_run;

	u8 ram[128_KB];
	std::vector<u8> SRAM;
	std::vector<u8> ROM;

};

