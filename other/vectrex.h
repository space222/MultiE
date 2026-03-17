#pragma once
#include <print>
#include "console.h"
#include "6809.h"

class Vectrex : public console
{
public:
	u32 fb_width() override { return 640; }
	u32 fb_height() override { return 480; }
	u32 fb_bpp() override { return 32; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	bool loadROM(std::string) override;
	void reset() override;	
	void run_frame() override;
	
	void cycle();
	
	struct {
		u8 DDRA=0, DDRB=0;
		u8 ORA=0, ORB=0;
		u8 SR=0, ACR=0, PCR=0;
		
		u8 T1L_L=0, T1L_H=0;
		u16 T1C=0;
		u8 T1_PB7=0;
		
		u8 T2L_L=0, T2L_H=0;
		u16 T2C=0;
		bool T2_oneshot = true;
		
		u8 IFR=0, IER=0;
	} pia;
	
	struct {
		s8 dx=0, dy=0;
		float px=0, py=0;
		u8 Z, doff=0;
		bool BLANK=false;
	} beam;

	u8 DAC=0;

	void sendMUX();
	
	void wr_pia(u16,u8);
	u8 rd_pia(u16);

	void write(u16, u8);
	u8 read(u16);

	u64 stamp, last_target;
	c6809 cpu;

	u8 bios[8_KB];
	u8 ram[1_KB];
	u8 cart[32_KB];
	
	float screen[640*480];
	
	u32 fbuf[640*480];
};

