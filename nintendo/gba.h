#pragma once
#include <vector>
#include <deque>
#include "console.h"
#include "Scheduler.h"
#include "arm7tdmi.h"
#include "LCDEngine.h"

class gba : public console
{
public:
	gba();
	u32 fb_width() override { return 240; }
	u32 fb_height() override { return 160; }
	u32 fb_bpp() override { return 32; }
	u8* framebuffer() override { return (u8*)&lcd.fbuf[0]; }
	
	bool loadROM(const std::string) override;
	void reset() override;
	void run_frame() override;
	
	u32 read(u32, int, ARM_CYCLE);
	void write(u32, u32, int, ARM_CYCLE);
	arm7tdmi cpu;
	Scheduler sched;
	void event(u64, u32) override;
	
	std::deque<s8> snd_fifo_a, snd_fifo_b;
	float sample;
	
	void write_io(u32, u32, int);
	u32 read_io(u32, int);
	
	u16 getKeys();
	bool halted;
	
	std::vector<u8> ROM;
	u8 bios[16*1024];
	u8 iwram[32*1024];
	u8 ewram[256*1024];
	u8 oam[1024];
	u8 palette[1024];
	u8 vram[96*1024];
	
	u32 bios_open_bus;
	
	LCDEngine lcd;
	bool frame_complete;
	
	u32 read_lcd_io(u32);
	u32 read_snd_io(u32) {return 0; }
	u32 read_dma_io(u32);
	u32 read_tmr_io(u32);
	u32 read_comm_io(u32) {return 0; }
	u32 read_pad_io(u32) {return 0; }
	u32 read_sys_io(u32);
	u32 read_memctrl_io(u32) {return 0; }

	void write_lcd_io(u32, u32);
	void write_snd_io(u32, u32) {return; }
	void write_dma_io(u32, u32);
	void write_tmr_io(u32, u32);
	void write_comm_io(u32, u32) {return; }
	void write_pad_io(u32, u32) {return; }
	void write_sys_io(u32, u32);
	void write_memctrl_io(u32, u32) {return; }
	
	u16 dmaregs[32];
	void exec_dma(int);
	
	u16 ISTAT, IMASK, IME;
	
	void check_irqs();
};

#define EVENT_SND_FIFO 1
#define EVENT_SND_OUT  2
#define EVENT_SCANLINE_START 3
#define EVENT_SCANLINE_RENDER 4
#define EVENT_FRAME_COMPLETE 5
#define EVENT_TMR0_CHECK 6
#define EVENT_TMR1_CHECK 7
#define EVENT_TMR2_CHECK 8
#define EVENT_TMR3_CHECK 9




