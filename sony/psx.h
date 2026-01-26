#pragma once
#include <vector>
#include <deque>
#include <span>
#include "console.h"
#include "Scheduler.h"
#include "r3000.h"

inline bool bcd_inc(u8& v, u8 rolat)
{
	v += 1;
	if( (v&0xf) == 10 ) v += 6;
	if( v >= rolat )
	{
		v = 0;
		return true;
	}
	return false;
}

inline bool bcd_dec(u8& v, u8 rolat)
{
	v -= 1;
	if( v == 0xff )
	{
		v = rolat;
		return true;
	}
	if( (v&0xf) == 0xf )
	{
		v -= 6;
	}
	return false;
}

struct mss
{
	u8 minutes=0, seconds=2, sectors=0;
	u32 track=1;
	u32 LBA=150;
	
	void inc()
	{
		if( bcd_inc(sectors, 0x75) ) if( bcd_inc(seconds, 0x60) ) bcd_inc(minutes, 0xff);
		calc_lba();
	}
	
	void dec()
	{
		if( bcd_dec(sectors, 0x74) ) if( bcd_dec(seconds, 0x59) ) bcd_dec(minutes, 0xff);
		calc_lba();
	}
	
	static mss from_lba(u32 L)
	{
		u32 min = L / (75*60);
		L -= min*(75*60);
		u32 seconds = L / 75;
		u32 sectors = L % 75;
		min = ((min/10)*16) + (min%10);
		seconds = ((seconds/10)*16) + (seconds%10);
		sectors = ((sectors/10)*16) + (sectors%10);
		return mss(min, seconds, sectors);	
	}
	
	u32 calc_lba()
	{
		u32 M = (minutes>>4)*10 + (minutes&0xf);
		u32 Snd = (seconds>>4)*10 + (seconds&0xf);
		u32 Sct = (sectors>>4)*10 + (sectors&0xf);
		LBA = (M*75*60) + (Snd*75) + Sct;
		return LBA;
	}
	
	mss() {}
	mss(u32 M, u32 secs, u32 sctors) 
		: minutes(M), seconds(secs), sectors(sctors), track(0), LBA(0) { calc_lba(); }
};

class psx : public console
{
public:
	psx();
	virtual ~psx();
	
	void run_frame() override;
	bool loadROM(std::string) override;
	u32 fb_width() override { return gpu_xres; } // 1024; }// (gpu_dispmode&0x10) ? gpu_xres:1024; }
	u32 fb_height() override { return gpu_yres; } // 512; } //(gpu_dispmode&0x10) ? gpu_yres:512; }
	u8* framebuffer() override { return fbuf.data(); } // (u8*)VRAM.data(); }// (gpu_dispmode&0x10) ? fbuf.data():((u8*)VRAM.data()); }
	u32 fb_bpp() override { return (gpu_dispmode&0x10)?24:16; }
	
	u32 fb_scale_w() override { return 640; }
	u32 fb_scale_h() override { return 480; }
	
	std::array<std::string, 8>& media_names() override;
	bool load_media(int, std::string) override;
	void reset() override;
	void reset_exe();
	
	void event(u64, u32) override;
	
	void write(u32 addr, u32 val, int size);
	u32 read(u32 addr, int size);
	
	std::vector<u16> VRAM;
	std::vector<u8> RAM;
	std::vector<u8> BIOS;
	std::vector<u8> SRAM;
	std::vector<u8> exe;
	u8 scratch[1024];
	
	u16 semitransp(u16, u8, u8, u8);
	void gpu_gp0(u32);
	void gpu_gp1(u32);
	u32 gpuread();
	bool gpu_read_active;
	u32 gpu_startx, gpu_starty, gpu_rx, gpu_ry, gpu_rw, gpu_rh, gpu_rval;
	u32 gpu_xres, gpu_yres, gpu_hdisp, gpu_vdisp;
	u32 gpu_data_rd();
	std::vector<u32> gpubuf;
	std::vector<u8> fbuf;
	u16 clut_cache[256];
	u16 last_clut_value;
	u32 gpustat, gpudata, texpage, gpu_curcmd, gpu_dispmode, gpu_dispstart;
	s16 area_x1, area_x2, area_y1, area_y2, offset_x, offset_y;
	s16 gpu_texmask_x, gpu_texmask_y, gpu_texoffs_x, gpu_texoffs_y;
	u8 gpu_field;
	
	u32 t0_val, t0_mode, t0_target;
	u32 t1_val, t1_mode, t1_target;
	u32 t2_val, t2_mode, t2_target, t2_div;
	void tick_timer_zero(u32);
	void tick_timer_one(u32);
	void tick_timer_two(u32);
	
	u32 DICR, DPCR;
	struct {
		u32 madr, bcr, chcr;
	} dchan[7];
	void dma_run_channel(u32);
	
	u16 I_STAT, I_MASK;
	
	const u32 IRQ_VBLANK = 1;
	const u32 IRQ_DMA = 8;
	const u32 IRQ_CD  =  4;
	const u32 IRQ_CONT = 0x80;
	const u32 IRQ_SIO = 0x100;
	const u32 IRQ_TIM0 = 0x10;
	const u32 IRQ_TIM1 = 0x20;
	const u32 IRQ_TIM2 = 0x40;
	const u32 IRQ_SPU = (1u<<9);
	
	struct P
	{
		s16 x, y;
		
		P(u32 v)
		{
			x = v&0x7ff; y = (v>>16)&0x7ff;
			x <<= 5; x >>= 5;
			y <<= 5; y >>= 5;
		}
		
		P operator-(const P& a) { return P(x-a.x, y-a.y); }
		
		P(s16 a, s16 b) : x(a), y(b) {}
		P(int a, int b) : x(s16(a)), y(s16(b)) {}
		P() {}
	};
	
	void draw_line_mono(u32 c, P v1, P v2);
	void set_texpage(u16);
	void reload_clut_cache();
	u16 sample_tex(u16 texpage, u8 u, u8 v);	
	void draw_tri(P v0, u32 r0, u32 g0, u32 b0,
                      P v1, u32 r1, u32 g1, u32 b1,
                      P v2, u32 r2, u32 g2, u32 b2);
	void draw_tex_tri(P vert0, u32 u0, u32 v0,
                      P vert1, u32 u1, u32 v1,
                      P vert2, u32 u2, u32 v2);
	void draw_tex_shaded_tri(P v0, u32 u0, u32 V0, P v1, u32 u1, u32 V1, P v2, u32 u2, u32 V2,
		u32 r0, u32 b0, u32 g0, u32 r1, u32 b1, u32 g1, u32 r2, u32 b2, u32 g2	);

	enum ADSR_STATE { ADSR_MUTED, ADSR_ATTACK, ADSR_DECAY, ADSR_SUSTAIN, ADSR_RELEASE };
	
	struct
	{
		u32 vxpitch, start, repeat, adsr, sustain, curadsr, Lcurvol, Rcurvol;
		u32 pcount;
		s16 Lvol, Rvol;
		u16 adsr_step;
		int adsr_vol, adsr_cyc;
		ADSR_STATE adsr_state;
		u32 curaddr, curnibble;
		s16 out, cur, old, older, oldest;
		s16 decoded_samples[28];
	} svoice[24];
	u32 spu_cycles;    // 33.8688 / 44100 = output current sample every 768 cycles
	u32 spu_write_addr;
	u32 spu_kon, spu_koff, spu_endx, spu_en_noise, spu_en_reverb, spu_pmon;
	u32 spu_stat, spu_ctrl, spu_irq_addr, spu_tran_ctrl;
	u16 spu_mainvol_L, spu_mainvol_R, spu_reverb_outvol_L, spu_reverb_outvol_R;
	u16 spu_cdvol_L, spu_cdvol_R;
	u32 spu_reverb_start;
	float spu_out;
	std::vector<float> spu_samples;
	void tick_spu();
	void spu_write(u32, u32, int);
	u32 spu_read(u32, int);
	void spu_decode_block(u32 voice);
	
	struct cdresult
	{
		u8 irq, rsize, state;
		u8 resp[16];
		cdresult() { state = 0; rsize = 1; irq = 3; }
	};
	u8 cd_resp[16];
	u8 cd_rpos, cd_rcomplete;
	std::vector<u8> cd_param;
	std::vector<cdresult> cd_irqq;
	u8 cd_index, cd_ie, cd_if, cd_mode;
	u8 cd_min, cd_s, cd_sectors;
	bool seek_pending;
	u8 cd_async_active, cd_active_cmd, cd_cmd_state;
	u8 cd_file, cd_channel;
	mss curloc, nextloc;
	u8* CDDATA;
	u32 CDDATA_size;
	u32 cd_offset;
	bool cd_lid_open;
	bool cd_active_read;
	void cd_write_1801(u8);
	void cd_write_1803(u8);
	void cd_cmd(u8);
	void cd_cmd_event(u64);
	void dispatch_cd_irq(const cdresult& R);

	void queue_cd_irq(u32 irq, const auto& resp)
	{
		cdresult c;
		memcpy(&c.resp[0], &resp[0], resp.size());
		c.irq = irq;
		c.rsize = resp.size();
		if( cd_if == 0 && (cd_ie & 7) && cd_rcomplete )
		{
			dispatch_cd_irq(c);
		} else {
			cd_irqq.push_back(c);
		}
	}
	
	struct
	{
		u16 pos;   // where in protocol sequence
		u8 device, cmd, sum, msb, lsb, flag; // device=1-cont. $81-card
		u32 cardaddr;
	} slot[2];
	u16 pad_stat, pad_ctrl, pad_baud, pad_mode, pad_tx, pad_rfifo_pos, pad_state;
	u64 pad_rx_fifo, pad_tx_fifo;
	void pad_write(u32 addr, u32 val);
	u32 pad_read(u32 addr);
	void pad_event(u64, u32);
	std::string mc0_fname;
	u8 memcard0[128*1024];
	u8 memcard1[128*1024];
	u8 memcard_read(u32,u8);
	u8 memcard_write(u32,u8);
	
	struct mb
	{
		u8 quant;
		int v[64];
		int output[64];
	};
	//u8 zagzig[64];
	mb blocks[6];
	u8 qtaby[64];
	u8 qtabc[64];
	s16 mdec_scale[64];
	u32 mdec_stat;
	std::vector<u16> mdec_data;
	u32 mdec_index;
	std::deque<u32> mdec_in;
	std::deque<u8> mdec_out;
	void mdec_cmd(u32);
	void mdec_run();
	void decode_block(u32, u8*);
	void mdec_write(u32, u32, int);
	u32 mdec_read(u32, int);
	
	std::vector<u8> exp1;
	
	//0x95, 0x05, 0x16, 0xc1 - cdrom controller date response	
	u64 stamp;
	r3000 cpu;
	Scheduler sched;
};

