#pragma once
#include "console.h"
#include "nvc.h"

class virtualboy : public console
{
public:
	~virtualboy();
	u32 fb_width() override { return 384; }
	u32 fb_height() override { return 224; }
	u8* framebuffer() override { return(u8*)&fbuf[0]; }

	void reset() override;
	void run_frame() override;
	bool loadROM(const std::string) override;

	void step();
	u32 read(u32, int);
	void write(u32, u32, int);
	
	u32 read_miscio(u32, int);
	void write_miscio(u32, u32, int);
	
	void draw_obj_group(u32);
	void draw_sprite(u32);
	void draw_normal_bg(u16* attr);
	void set_fb_pixel(u32 lr, int x, int y, u32 p);
	
	nvc cpu;
	u64 stamp, last_target;
	u16 INTPND, INTENB, DPSTTS, padkeys;
	u16 objctrl[4];
	int which_buffer;
	
	std::vector<u8> ROM;	
	u8 ram[64*1024];
	u8 vram[256*1024];
	
	u32 fbuf[384*224];
};


