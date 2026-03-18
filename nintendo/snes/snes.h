#pragma once
#include <cstdio>
#include <print>
#include <vector>
#include "console.h"
#include "65816.h"
//#include "spc700.h"

class snes : public console
{
public:
	~snes()
	{
	}
	
	snes() { setVsync(1); }

	u32 fb_bpp() override { return 16; } // bgr555
	u32 fb_width() override { return 256; }
	u32 fb_height() override { return 224; }
	//u32 fb_scale_h() override { return 480; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	void reset() override 
	{ 
		cpu.E=1;
		cpu.F.b.M = 1;
		cpu.F.b.X = 1;
		cpu.X.v = cpu.Y.v = 0;
		cpu.pc = cpu.bus_read(0xfffc); 
		cpu.pc |= cpu.bus_read(0xfffd)<<8; 
		cpu.opc = cpu.bus_read(cpu.pc); 
		cpu.pb=0; 
		cpu.S.v = 0x1FFF;
		cpu_run = cpu.run(); 
	}

	void run_frame() override;
	bool loadROM(std::string) override;
	
	void do_master_cycles(s64 master_cycles);
	
	u8 read(u32);
	void write(u32, u8);
	u8 io_read(u8, u32);
	void io_write(u8, u32, u8);
	
	void run_dma(u32 cn);
	
	struct {
		u8 memsel=0, nmitimen=0, wrio=0;
		u8 wrmpya=0, wrmpyb=0, wrdivl=0, wrdivh=0, wrdivb=0;
		u8 htimel=0, htimeh=0, vtimel=0, vtimeh=0, mdmaen=0, hdmaen=0;
		u8 wmaddl=0, wmaddm=0, wmaddh=0;
	} io;
	
	struct {
		s64 master_cycles=0;
		bool frame_complete=false;
		u32 scanline=0;
		
		u8 inidisp=0, bgmode=0, mosaic=0, bg1sc=0, bg2sc=0, bg3sc=0, bg4sc=0;
		u8 bg12nba=0, bg34nba=0, vmain=0;
		u16 bg1hofs=0, bg1vofs=0, bg2hofs=0, bg2vofs=0, bg3hofs=0, bg3vofs=0, bg4hofs=0, bg4vofs=0;
		u8 bgofs_latch=0, bghofs_latch=0;
		
		u8 w12sel=0, w34sel=0, wobjsel=0, wh0=0, wh1=0, wh2=0, wh3=0, wbglog=0, wobjlog=0;
		u8 tm=0, ts=0, tmw=0, tsw=0;
		u8 cgadd=0;
		u8 cgwsel=0, cgadsub=0, coldata=0, setini=0, mpyl=0, mpym=0, mpyh=0, slhv=0;
		
		u16 m7a=0, m7b=0, m7c=0, m7d=0, m7x=0, m7y=0;
		u8 m7latch=0;
		
		u8 wmaddh=0, wmaddl=0;
		
		u8 objsel=0, oamaddh=0, oamaddl=0;
		u8 oam_latch=0;
		u16 oamadd=0,internal_oamadd=0;
		
		u8 cgram_byte=0;
		u8 cgram_latch=0;
		u16 cgram[256];
		
		u8 rdnmi=0;
		
		u8 dmaregs[0x80];
		u8 vram[64_KB];
		u8 oam[512];
		u8 oamhi[32];
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
	
	struct {
		u8 to_cpu[4];
		u8 to_spc[4];

		u32 tinternal[3]={0,0,0};
		u32 target[3]={0,0,0};
		u32 tout[3]={0,0,0};

		u8 dsp_regs[0x100];
		u32 glblcnt = 0;

		enum phases { ADSR_ATTACK=0, ADSR_DECAY, ADSR_SUSTAIN, ADSR_RELEASE, ADSR_GAIN };
		struct {
			u32 block_addr=0;
			u32 loop_addr=0;
			u16 pcnt=0;
			s16 decoded[16];
			u32 decoded_ind=0;
			s16 oldest=0, older=0, old=0, cur=0, out=0;
			s16 env_level;
			int phase=ADSR_RELEASE;
		} voice[8];
	} apu;
	u8 spcram[64_KB];
	

	u16 fbuf[512*480];

	c65816 cpu;
	Yieldable cpu_run;

	u8 ram[128_KB];
	std::vector<u8> SRAM;
	std::vector<u8> ROM;

};

