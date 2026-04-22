#pragma once
#include <print>
#include <vector>
#include "console.h"
#include "Scheduler.h"
#include "arm7tdmi.h"
#include "arm946e.h"

class nds : public console
{
public:
	u32 fb_width() override { return 256; }
	u32 fb_height() override { return 192*2; }
	u32 fb_bpp() override { return 16; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	void run_frame() override;	
	void reset() override;
	bool loadROM(std::string) override;

	arm7tdmi arm7;
	arm946e  arm9;
	
	u32 arm9_fetch(u32, int, ARM_CYCLE);
	
	u32 arm7_io_read(u32, int);
	void arm7_io_write(u32, u32, int);
	u32 arm9_io_read(u32, int);
	void arm9_io_write(u32, u32, int);
		
	u32 arm7_read(u32, int, ARM_CYCLE);
	void arm7_write(u32, u32, int, ARM_CYCLE);

	u32 arm9_read(u32, int, ARM_CYCLE);
	void arm9_write(u32, u32, int, ARM_CYCLE);
	
	void arm9_raise_irq(u32 bit);
	void arm9_clear_irq(u32 bit);
	void arm7_raise_irq(u32 bit);
	void arm7_clear_irq(u32 bit);
	const u32 IRQ_IPCSYNC_BIT = BIT(16);
	
	struct {
		u32 IME, IF, IE;
		u8 halted;
	} irq7, irq9;
	
	u32 keys();
	u32 keystate = 0xffffFFFFu;
	
	struct {
		u8 to_arm7, to_arm9;
		u16 ipcsync7, ipcsync9;
	} ipc;

	std::vector<u8> ROM;

	u8 mainram[4_MB];
	u8 itcm[32_KB];
	u8 dtcm[16_KB];
	u8 vram[656_KB];
	u8 arm9_bios[32_KB];
	u8 arm7_bios[16_KB];
	u8 arm7_wram[96_KB];
	u16 fbuf[256*192*2];
};

