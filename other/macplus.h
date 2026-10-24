#pragma once
#include <deque>
#include "moof.h"
#include "console.h"
#include "68k.h"

class macplus : public console
{
public:
	void reset() override;
	void run_frame() override;
	bool loadROM(const std::string) override;
	u32 fb_width() override { return 512; }
	u32 fb_height() override { return 342; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	void keysym_down(int sc) override { key_event(sc, 0); }
	void keysym_up(int sc) override { key_event(sc, 0x80); }
	
	void key_event(int sc, u8 orit);
	
	void mouse_move(u32 x, u32 y, int relx, int rely) override;
	
	void mouse(u32, u32 state) override { mouse_down = state; }

	void write(u32 addr, u32 v, int size);
	u32 read(u32 addr, int size);
	
	m68k cpu;
	u64 stamp, last_target, target;
	void step();
	
	u32 fbuf[512*342];

	u8 mainRAM[4*1024*1024];
	u8 bios[512*1024];
	
	bool in_reset{};
	u8 vblank=0;
	MOOF* floppyA{};
	u8 floppyA_track{3}, floppyA_tdir{}, floppyA_data{}, floppyA_motor{};
	u8 tach{};
	u32 curtrack_bitcount{0};
	u32 floppy_div{0};

	u8 iwm_q6{}, iwm_q7{}, iwm_enable{}, iwm_mode{}, iwm_ca2{}, iwm_ca1{}, iwm_ca0{}, iwm_strobe{}, iwm_internal{};
	u8 via_vIFR{}, via_vIER{}, via_vPCR{}, via_vACR{}, via_vBufA{}, via_vSR{};
	u8 via_tmr2latch{}, via_tmr2irq{};
	u16 via_tmr2{};
	u16 via_tmr2div{};

	bool mouse_down{};
	int tachdiv = 0;

	u64 onesec = 0;

	SDL_Window* MainWindow{};
	SDL_Renderer* MainRend{};
	SDL_Texture* MainTex{};
	std::deque<u8> keys;
	u8 last_key{0x7B};
	u8 kb_cmd = 0;
	int kb_shift = 0;

	void write_iwm_cmd();
	u8 read_sense();
};



