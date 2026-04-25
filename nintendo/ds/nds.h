#pragma once
#include <print>
#include <vector>
#include <deque>
#include "console.h"
#include "Scheduler.h"
#include "arm7tdmi.h"
#include "arm946e.h"

class nds : public console
{
public:
	nds() : sched(this) {}
	u32 fb_width() override { return 256; }
	u32 fb_height() override { return 192*2; }
	u32 fb_bpp() override { return 16; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	void run_frame() override;	
	void reset() override;
	bool loadROM(std::string) override;
	void event(u64 oldstamp, u32 code) override;
	
	Scheduler sched;
	const u32 EVENT_HBLANK_START = 1;
	const u32 EVENT_NEXT_SCANLINE = 2;
	bool frame_complete;
	
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
	const u32 IRQ_IPC_SEND_EMPTY = BIT(17);
	const u32 IRQ_IPC_RECV_EMPTY = BIT(18);
	const u32 IRQ_VBLANK_BIT = BIT(0);
	const u32 IRQ_HBLANK_BIT = BIT(1);
	const u32 IRQ_VCOUNT_BIT = BIT(2);
	
	struct {
		u32 IME, IF, IE;
	} irq7, irq9;
	
	u32 keys();
	u32 keystate = 0xffffFFFFu;
	
	struct {
		u8 to_arm7, to_arm9;
		u16 ipcsync7, ipcsync9;
		std::deque<u32> q2arm7, q2arm9;
		u32 last_q2arm7, last_q2arm9;
		
		u32 fifocnt7, fifocnt9;
	} ipc;
	
	struct {
		u64 div_num, div_den, div_quot, div_rem;
		u64 sqrt_param;
		u32 sqrt_res;
		u32 divcnt, sqrtcnt;
	} dsmath;
	void dsmath_div();
	void dsmath_sqrt();
	
	u8 wramcnt;
	
	struct {
		u32 scanline;
		u32 stat;
	} disp;
	const u32 DISPSTAT_VBLANK_FLAG = BIT(0);
	const u32 DISPSTAT_HBLANK_FLAG = BIT(1);
	const u32 DISPSTAT_VCOUNT_FLAG = BIT(2);
	const u32 DISPSTAT_VBLANK_IRQ_EN = BIT(3);
	const u32 DISPSTAT_HBLANK_IRQ_EN = BIT(4);
	const u32 DISPSTAT_VCOUNT_IRQ_EN = BIT(5);
	
	u8* engineA_bg[512 / 4];
	u8* engineB_bg[128 / 4];
	u8* engineA_obj[256 / 4];
	u8* engineB_obj[128 / 4];
	u8 vmap_bytes[11];
	void remap_vram();

	std::vector<u8> ROM;

	u8 mainram[4_MB];
	u8 itcm[32_KB];
	u8 dtcm[16_KB];
	u8 vram[656_KB];
	u8 arm9_bios[32_KB];
	u8 arm7_bios[16_KB];
	u8 shared_wram[32_KB];
	u8 arm7_wram[64_KB];
	u16 fbuf[256*192*2];
};

