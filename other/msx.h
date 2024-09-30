#pragma once
#include <string>
#include <cstdio>
#include "console.h"
#include "z80.h"
#include "TMS9918A.h"
#include "AY-3-8910.h"

class msx : public console
{
public:
	msx();
	~msx();
	
	virtual void key_down(int sc)
	{
		if( sc == SDL_SCANCODE_F12 ) 
		{
			printf("MSX: Cassette in write mode\n");
			casrd = false;
			tap.clear();
			caspos = 0;
		}
		if( sc == SDL_SCANCODE_F11 )
		{
			printf("MSX: Cassette in read mode\n");
			casrd = true;
			caspos = 0;
		}
	}
	
	u32 fb_width() override { return vdp.fb_width(); }
	u32 fb_scale_w() override { return 256; }
	u32 fb_height() override { return 192; }
	void reset() override;
	bool loadROM(const std::string) override;
	void run_frame() override;
	u8* framebuffer() override { return vdp.framebuffer(); }
	
	z80 cpu;
	TMS9918A vdp;
	AY_3_8910 psg;
	
	u8 read(u16);
	void write(u16, u8);
	void out(u16, u8);
	u8 in(u16);
	
	u64 last_target, stamp, sample_cycles;
	
	u8 ram[64*1024];
	u8 rom[64*1024];
	u8 bios[32*1024];
	u8 slots[4];
	u8 slotreg;
	
	u8 ppi_c;
	u8 kbrow;
	u64 base_cas_stamp;
	std::vector< std::pair<u64, u8> > tap;
	bool casrd;
	u32 caspos;
};

