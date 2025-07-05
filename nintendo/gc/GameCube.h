#pragma once
#include <print>
#include <vector>
#include "console.h"
#include "gekko.h"

class GameCube : public console
{
public:
	u32 fb_width() override { return 640; }
	u32 fb_height() override { return 480; }
	//u32 fb_bpp() override { return 16; }
	void run_frame() override;
	void reset() override;
	bool loadROM(std::string) override;
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	void key_down(int) override
	{
		//FILE* fp = fopen("intr.bin", "wb");
		//fwrite(mem1+0x500, 512, 1, fp);
		//fclose(fp);
		std::println("pc = ${:X}", cpu.pc);
	}

	u32 fetch(u32);
	u32 read(u32, int);
	void write(u32, u32, int);
	
	double read_double(u32);
	void write_double(u32, double);
	
	void setINTSR(u32);
	void clearINTSR(u32);
	
	void descramble(u8*, u32);
	
	struct {
		u32 EXI2DATA, EXI2CSR, EXI2CR;
		u32 EXI0DATA, EXI0CSR, EXI0CR, EXI0MAR, EXI0LENGTH;
	} exi;
	void exi_write(u32,u32,int);
	u32 exi_read(u32,int);
	
	struct {
		u32 dpv, fbaddr;
		union __attribute__((packed)) {
			struct __attribute__((packed))
			{
				unsigned int hpos : 10;
				unsigned int pad0 : 6;
				unsigned int vpos : 10;
				unsigned int pad1 : 2;
				unsigned int e : 1;
				unsigned int pad2 : 2;
				unsigned int i : 1;
			} b;
			u32 v;
		} di[4];
	} vi;
	
	struct {
		u32 INTSR;
		u32 INTMR;
	} pi;
	
	
	int debug_type;

	gekko cpu;
	
	u8 mem1[24*1024*1024];
	u32 fbuf[720*480];
	std::vector<u8> ipl, ipl_clear;
};

#define INTSR_VI_BIT BIT(8)



