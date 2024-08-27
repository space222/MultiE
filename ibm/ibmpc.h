#pragma once
#include <vector>
#include <deque>
#include "console.h"
#include "80286.h"

class ibmpc : public console
{
public:
	ibmpc();
	void reset() override;
	void run_frame() override;
	u8* framebuffer() override { return (u8*) fbuf.data(); }
	u32 fb_width() override { return 720; }
	u32 fb_height() override { return 480; }
	bool loadROM(std::string) override;
	std::array<std::string, 8>& media_names() override;

	c80286 cpu;

	u8 read(u32);
	void write(u32, u8);
	u16 port_in(u16, int);
	void port_out(u16, u16, int);
	
	struct {
		u8 rdlatch[4];
		bool yay;
		u8 index3D4;
		u8 reg3D4[0x100];
		u8 index3CE;
		u8 reg3CE[0x100];
		u8 index3C4;
		u8 reg3C4[0x100];
		int index3C0;
		u8 reg3C0[0x100];
		u8 misc_output, dac_mask, dac_state;
		int dac_write_index, dac_read_index;
		u8 dac[0x100*3];
	} vga;
		
	struct
	{
		u8 OUT;
		bool latch;
		u8 hilo_r, hilo_w;
		u32 CE;
		u16 OL;
		u16 initial;
		u8 ctrl, gate;
		u8 reset;
	} pitchan[3];
	u32 pitdiv;
	
	u8 PPI_B;
	
	void pitcycle();
	
	void vga_port_write(u16, u16, int);
	u8 vga_port_read(u16);
	void vga_write_vram(u32, u8);
	u8 vga_read_vram(u32);
	
	void fdc_write(u16,u8);
	u8 fdc_read(u16);
	std::vector<u8> fdc_cmd_fifo;
	std::deque<u8> fdc_res_fifo;
	
	void pic_write(u16,u8);
	u8 pic_read(u16);
	
	void pit_write(u8, u8);
	u8 pit_read(u8);

	std::vector<u32> fbuf;
	std::vector<u8> RAM;
	std::vector<u8> ROM;
	std::vector<u8> gfx;
};



