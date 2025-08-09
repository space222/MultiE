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
	/*void key_down(int s) override
	{
		if( s == SDL_SCANCODE_ESCAPE )
		{
			for(u32 i = 0; i < sched.events.size(); ++i)
			{
				std::println("event {}, stamp ${:X}", sched.events[i].code, sched.events[i].stamp);
			}		
		}
	}*/

	gba();
	~gba();
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
	Scheduler *sched;
	void event(u64, u32) override;
	
	std::deque<s8> snd_fifo_a, snd_fifo_b;
	float pcmA,pcmB;
	u8 fifoa;
	
	void write_io(u32, u32, int);
	u32 read_io(u32, int);
	
	u16 getKeys();
	bool halted;
	
	std::vector<u8> ROM;
	u8 bios[16*1024];
	u8 iwram[32*1024];
	u8 ewram[256*1024];
	u8 palette[1024];
	u8 vram[96*1024];
	u8 oam[1024];
	
	u8 save[0x20000];
	bool save_written;
	u32 save_type, save_size;
	std::string savefile;
	u32 flash_state, flash_bank, flash_cmd;
		
	u32 eeprom_read();
	void eeprom_write(u8);
	u32 eeprom_state, eeprom_mode, eeprom_count;
	u32 eeprom_addr;
	u64 eeprom_out;
	u32 bios_open_bus;
	
	LCDEngine lcd;
	bool frame_complete;
	
	u32 read_lcd_io(u32);
	u32 read_snd_io(u32);
	u32 read_dma_io(u32);
	u32 read_tmr_io(u32);
	u32 read_comm_io(u32) {return 0; }
	u32 read_pad_io(u32) {return 0; }
	u32 read_sys_io(u32);
	u32 read_memctrl_io(u32) {return 0; }

	void write_lcd_io(u32, u32);
	void write_snd_io(u32, u32);
	void write_dma_io(u32, u32);
	void write_tmr_io(u32, u32);
	void write_comm_io(u32, u32) {return; }
	void write_pad_io(u32, u32) {return; }
	void write_sys_io(u32, u32);
	void write_memctrl_io(u32, u32) {return; }
	
	u16 dmaregs[32];
	u32 internsrc1, internsrc2;
	void exec_dma(int);
	
	u16 ISTAT, IMASK, IME;
	
	struct {
		u64 last_read;
		u16 value;
		u16 reload;
		u16 ctrl;
	} tmr[4];
	void catchup_timer(u32);
	void tick_overflow_timer(u32);
	void timer_event(u64,u32);
	
	bool snd_master_en;
	u16 dma_sound_mix;
	void snd_a_timer();
	void snd_b_timer();
	
	bool gpio_reads_en;
	u8 gpio_dir, gpio_data;
	void gpio_write(u32, u32);
	u8 gpio_read(u32);
	
	u8 rtc_cmd, rtc_ctrl;
	u32 rtc_state;
	u64 rtc_out;
	
	void check_irqs();
};

#define EVENT_SND_OUT  2
#define EVENT_SCANLINE_START 3
#define EVENT_SCANLINE_RENDER 4
#define EVENT_FRAME_COMPLETE 5
#define EVENT_TMR0_CHECK 6
#define EVENT_TMR1_CHECK 7
#define EVENT_TMR2_CHECK 8
#define EVENT_TMR3_CHECK 9

#define SAVE_TYPE_UNKNOWN 0
#define SAVE_TYPE_SRAM 1
#define SAVE_TYPE_EEPROM 2
#define SAVE_TYPE_FLASH 3






