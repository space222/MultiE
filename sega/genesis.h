#pragma once
#include <string>
#include <vector>
#include "console.h"
#include "z80.h"
#include "68k.h"
#include "SN79489.h"
#include "SH2.h"

class genesis : public console
{
public:
	genesis() { vdp_width = 320; setVsync(0); }
	u32 fb_width() override { return vdp_width; }
	u32 fb_scale_w() override { return 320; }
	u32 fb_height() override { return 224; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	
	bool loadROM(const std::string) override;
	void reset() override;
	void run_frame() override;
	
	u32 read(u32, int);
	void write(u32, u32, int);
	u8 z80_read(u16);
	void z80_write(u16, u8);
	
	bool pal;
	
	z80 spu;
	m68k cpu;
	SN79489 psg;
	
	u64 last_target;
	u64 stamp, spu_stamp;
	
	void vdp_ctrl(u16);
	void vdp_data(u16);
	u16 vdp_read();
	void draw_line(u32);
	void vdp_vram2vram();
	void eval_sprites(u32);
	void render_sprite(u32 y, u32 x, u32 hs, u32 vs, u16 entry);
	bool vdp_in_window(u32 line, u32 px);
	bool vdp_latch, fill_pending;
	u8 vreg[0x20];
	u16 vdp_addr, vdp_stat;
	u32 vdp_width;
	u8 vdp_cd;
	u8 sprbuf[320];
	
	u32 pcycle, pcycle2;
	u16 key1, key2, key3;
	u16 pad2_key1, pad2_key2, pad2_key3;
	u16 pad2_data, pad1_data, pad1_ctrl, pad2_ctrl;
	void pad_get_keys();
	u16 getpad1();
	u16 getpad2();
	
	u16 z80_busreq, z80_reset;
	u32 z80_bank;
	u32 sample_cycles;
	
	u64 psg_stamp;
	
	std::vector<u8> ROM;
	u8 RAM[0x10000];
	u8 VRAM[0x10000];
	u8 CRAM[128];
	u8 VSRAM[128];
	u8 ZRAM[0x2000];
	u8 save[0x10000];
	u32 fbuf[320*224];

	// 32X stuff
	SH2 cpu32x[2];
	u8 fb32x[256*1024];
	u8 sdram[256*1024];
	u8 vecrom[256];
	u8 uppercache[0x1000];
	u8 comms[16];
	u8 bios32xS[0x800];
	u8 bios32xM[0x800];
	void write32x(u32, u32, int);
	u32 read32x(u32, int);
	void sh2master_write(u32,u32,int);
	void sh2slave_write(u32,u32,int);
	u32 sh2master_read(u32,int);
	u32 sh2slave_read(u32,int);
	u32 ADEN, RES, RV, FM, bank9, autofill_len, autofill_addr, autofill_val;
	u16 bmpmode, fb_ctrl, fb_ctrl_fsnext;
};

#define PAD_DATA_DEFAULT 0x40



