#pragma once
#include <deque>
#include "console.h"
#include "sh4.h"
#include "util.h"
#include "Scheduler.h"

class dreamcast : public console
{
public:
	dreamcast() : sched(this) {}
	~dreamcast() { std::println("Ended @PC = ${:X}", cpu.pc); }
	
	u32 fb_bpp() override;
	u8* framebuffer() override { return (u8*)&fbuf[0]; }
	u32 fb_width() override;
	u32 fb_height() override;
	u32 fb_scale_w() override;
	u32 fb_scale_h() override;	
	void reset() override;
	void run_frame() override;
	bool loadROM(std::string) override;

	sh4 cpu;
	u64 stamp, frame_start;
	
	u32 storeq[16];
	void write_storeQ(u32);
	
	struct {
		u32 CCR, QACR0, QACR1;
	
		u32 TOCR, TSTR;
		u32 TCOR0, TCNT0, TCR0;
		u32 TCOR1, TCNT1, TCR1;
		u32 TCOR2, TCNT2, TCR2;
		u32 TCPR2;
		u64 last_tmr0_read, last_tmr1_read, last_tmr2_read;
		
		u8 SCBRR2;
		
		u8 WTCNT;
		u16 RFCR;
		
		u32 IPRA, IPRB, IPRC;
		
		u32 CHCR3;
		
		u32 PDTRA, PCTRA;
	} intern;
	
	void catch_up_timer(u32);
	void timer_underflow(u64, u32);
	const u32 TMR_IRQ_ACTIVE = BIT(8)|BIT(5);
	
	u8 RAM[16_MB];
	u8 vram[8_MB];
	u8 bios[2_MB];
	u8 flashrom[128_KB];
	u8 sndram[2_MB];	
	u8 opcache[8_KB];
	
	u64 read(u32, u32);
	void write(u32, u64, u32);
	
	u64 io_read(u32, u32);
	void io_write(u32, u64, u32);
	
	void check_irqs();
	
	u64 cpureg_read(u32, u32);
	void cpureg_write(u32,u64,u32);
	u64 timer_read(u32, u32);
	void timer_write(u32,u64,u32);
	
	const u32 GD_ALT_STAT = 0x5F7018;
	const u32 GD_STAT_CMD = 0x5F709C;
	u64 gdread(u32,u32);
	void gdwrite(u32,u64,u32);
	void gdevent(u64,u32);
	struct {
		u8 stat;
		u8 dev_ctrl;
		
		
		u8 curcmd;
	} gd;

	u32 fbuf[640*480];
	
	Scheduler sched;
	void event(u64, u32) override;
	const u32 HBLANK_IN = 0x100;
	const u32 VBLANK_IN = 0x101;
	const u32 VBLANK_OUT = 0x102;	
	
	const u32 CCR_ORA_BIT = BIT(5);
	const u32 CCR_OIX_BIT = BIT(7);
	const u32 CCR_OCE_BIT = BIT(0);
	
	const u32 TMR0_UNDERFLOW = 1;
	const u32 TMR1_UNDERFLOW = 2;
	const u32 TMR2_UNDERFLOW = 3;
	
	const u32 SB_MDSTAR_ADDR = 0x5F6C04;
	const u32 SB_MDST_ADDR = 0x5F6C18;
		
	const u32 HOLLY_ID_ADDR = 0x5F8000;
	const u32 HOLLY_SOFT_RESET_ADDR = 0x5F8008;
	const u32 VO_BORDER_COLOR_ADDR = 0x5F8040;
	const u32 SB_ISTNRM_ADDR = 0x5F6900;
	const u32 VO_CONTROL_ADDR = 0x5F80E8;
	const u32 FB_R_CTRL_ADDR = 0x5F8044;
	const u32 SPAN_SORT_CFG_ADDR = 0x5F8030;
	const u32 FOG_COL_RAM_ADDR = 0x5F80B0;
	const u32 FOG_COL_VERT_ADDR = 0x5F80B4;
	const u32 FOG_DENSITY_ADDR = 0x5F80B8;
	const u32 SB_LMMODE0_ADDR = 0x5F6884;
	const u32 SB_LMMODE1_ADDR = 0x5F6888;
	const u32 SPG_VBLANK_INT_ADDR = 0x5F80CC;
	const u32 FB_X_CLIP_ADDR = 0x5F8068;
	const u32 FB_Y_CLIP_ADDR = 0x5F806C;
	const u32 FB_BURSTCTRL_ADDR = 0x5F8110;
	const u32 SPG_CONTROL_ADDR = 0x5F80D0;
	const u32 SPG_HBLANK_ADDR = 0x5F80D4;
	const u32 SPG_LOAD_ADDR = 0x5F80D8;
	const u32 SPG_VBLANK_ADDR = 0x5F80DC;
	const u32 SPG_WIDTH_ADDR = 0x5F80E0;
	const u32 FB_W_SOF1_ADDR = 0x5F8060;
	const u32 FB_W_SOF2_ADDR = 0x5F8064;
	const u32 FB_W_CTRL_ADDR = 0x5F8048;
	const u32 FB_W_LINESTRIDE_ADDR = 0x5F804C;
	const u32 FB_R_SOF1_ADDR = 0x5F8050;
	const u32 FB_R_SOF2_ADDR = 0x5F8054;
	const u32 FB_R_SIZE_ADDR = 0x5F805C;
	const u32 SPG_HBLANK_INT_ADDR = 0x5F80C8;
	const u32 FPU_SHAD_SCALE_ADDR = 0x5F8074;
	const u32 FPU_CULL_VAL_ADDR = 0x5F8078;
	const u32 FPU_PARAM_CFG_ADDR = 0x5F807C;
	const u32 HALF_OFFSET_ADDR = 0x5F8080;
	const u32 FPU_PERP_VAL_ADDR = 0x5F8084;
	const u32 Y_COEFF_ADDR = 0x5F8118;
	const u32 ISP_BACKGND_D_ADDR = 0x5F8088;
	const u32 ISP_BACKGND_T_ADDR = 0x5F808C;
	const u32 FOG_CLAMP_MAX_ADDR = 0x5F80BC;
	const u32 FOG_CLAMP_MIN_ADDR = 0x5F80C0;
	const u32 TEXT_CONTROL_ADDR = 0x5F80E4;
	const u32 VO_STARTX_ADDR = 0x5F80EC;
	const u32 VO_STARTY_ADDR = 0x5F80F0;
	const u32 SCALER_CTL_ADDR = 0x5F80F4;
	const u32 SB_G1CRC_ADDR = 0x5F7490;
	const u32 SB_G1CWC_ADDR = 0x5F7494;
	const u32 SB_G1GDRC_ADDR = 0x5F74A0;
	const u32 SB_G1GDWC_ADDR = 0x5F74A4;
	const u32 SB_G2DSTO_ADDR = 0x5F7890;
	const u32 SB_G2TRTO_ADDR = 0x5F7894;
	const u32 SB_FFST_ADDR = 0x5F688C;
	const u32 SB_IML2NRM_ADDR = 0x5F6910;
	const u32 SB_IML4NRM_ADDR = 0x5F6920;
	const u32 SB_IML6NRM_ADDR = 0x5F6930;
	const u32 SB_IML2EXT_ADDR = 0x5F6914;
	const u32 SB_IML4EXT_ADDR = 0x5F6924;
	const u32 SB_IML6EXT_ADDR = 0x5F6934;
	const u32 SB_IML2ERR_ADDR = 0x5F6918;
	const u32 SB_IML4ERR_ADDR = 0x5F6928;
	const u32 SB_IML6ERR_ADDR = 0x5F6938;
	const u32 SB_ISTEXT_ADDR = 0x5F6904;
	const u32 SB_ISTERR_ADDR = 0x5F6908;
	const u32 SB_C2DSTAT_ADDR = 0x5F6800;
	const u32 SB_C2DLEN_ADDR = 0x5F6804;
	const u32 SB_SDSTAW_ADDR = 0x5F6810;
	const u32 SB_SDBAAW_ADDR = 0x5F6814;
	const u32 SB_SDWLT_ADDR = 0x5F6818;
	const u32 SB_DBREQM_ADDR = 0x5F6840;
	const u32 SB_SDLAS_ADDR = 0x5F681C;
	const u32 SB_BAVLWC_ADDR = 0x5F6844;
	const u32 SB_C2DPRYC_ADDR = 0x5F6848;
	const u32 SB_C2DMAXL_ADDR = 0x5F684C;
	const u32 SB_RBSPLT_ADDR = 0x5F68A0;
	const u32 UNKNOWN_REG_68A4 = 0x5F68A4;
	const u32 SPG_STATUS_ADDR = 0x5F810C;
	const u32 STARTRENDER_ADDR = 0x5F8014;
	const u32 REGION_BASE_ADDR = 0x5F802C;
	const u32 PARAM_BASE_ADDR = 0x5F8020;
	const u32 SB_ADSUSP_ADDR = 0x5F781C;
	const u32 SB_E1SUSP_ADDR=  0x5F783C;
	const u32 SB_E2SUSP_ADDR = 0x5F785C;
	const u32 SB_DDSUSP_ADDR = 0x5F787C;
	const u32 SB_C2DST_ADDR = 0x5F6808;
	
	const u32 SB_GDST_ADDR = 0x5F7418;
	
	const u32 TA_OL_BASE_ADDR = 0x5F8124;
	const u32 TA_ISP_BASE_ADDR = 0x5F8128;
	const u32 TA_OL_LIMIT_ADDR =0x5F812C;
	const u32 TA_ISP_LIMIT_ADDR = 0x5F8130;
	
	const u32 TA_GLOB_TILE_CLIP_ADDR = 0x5F813C;
	const u32 TA_ALLOC_CTRL_ADDR = 0x5F8140;
	const u32 TA_LIST_INIT_ADDR = 0x5F8144;
	
	const u32 TA_NEXT_OPB_INIT_ADDR = 0x5F8164;
	
	struct ta_vertex
	{
		float x, y, z;
		float u, v;
		float r, g, b, a;
	};
	
	std::deque<u32> ta_q;
	std::deque<ta_vertex> ta_vertq;
	u32 ta_prim_type, ta_col_type, ta_obj_ctrl, ta_tex_ctrl, ta_tsp_mode;
	int ta_active_list;
	bool ta_clear;
	void ta_draw_tri();
	void ta_vertex_in();
	void ta_list_init();
	void ta_input(u32);
	void ta_run();
	void start_render();
	void render_opaque(u32);
	
	void maple_dma();
	void maple_port1(u32 dest, u32* msg, u32 len);
	void maple_port2(u32 dest, u32* msg, u32 len);
	u32 maple_keys1();
	
	bool debug_on;
	void key_down(int k)
	{
		if( k == SDL_SCANCODE_ESCAPE ) debug_on = !debug_on;
		if( k == SDL_SCANCODE_F ) 
		{ 
			intern.TCR2 |= TMR_IRQ_ACTIVE;
			intern.TCR1 |= TMR_IRQ_ACTIVE;
			intern.TCR0 |= TMR_IRQ_ACTIVE;
			std::println(stderr, "${:X}, ${:X}, ${:X}", intern.TCR0, intern.TCR1, intern.TCR2); 
		}
		if( k == SDL_SCANCODE_F1 ) 
		{ 
			holly.sb_istnrm |= 1;
		}
		if( k == SDL_SCANCODE_F2 ) 
		{ 
			holly.sb_istnrm |= 2;
		}
		if( k == SDL_SCANCODE_F3 ) 
		{ 
			holly.sb_istnrm |= 4;
		}
		if( k == SDL_SCANCODE_F4 ) 
		{ 
			holly.sb_istnrm |= 8;
		}
		if( k == SDL_SCANCODE_F5 ) 
		{ 
			holly.sb_istnrm |= 0x10;
		}
		if( k == SDL_SCANCODE_F6 ) 
		{ 
			holly.sb_istnrm |= 0x20;
		}
		if( k == SDL_SCANCODE_F7 ) 
		{ 
			holly.sb_istnrm |= 0x40;
		}
		if( k == SDL_SCANCODE_F8 ) 
		{ 
			holly.sb_istnrm |= 0x80;
		}
		if( k == SDL_SCANCODE_F9 ) 
		{ 
			holly.sb_istnrm |= 0x100;
		}
		if( k == SDL_SCANCODE_F10 ) 
		{ 
			holly.sb_istnrm |= 0x200;
		}
		if( k == SDL_SCANCODE_F11 ) 
		{ 
			holly.sb_istnrm |= 0x400;
		}
	}
	
	struct {
		u32 ol_base, ol_limit;
		u32 isp_base, isp_limit;
		u32 glob_tile_clip, alloc_ctrl;
		u32 next_opb_init;
	} ta;
	
	struct {
		u32 backgnd_d, backgnd_t;
	} isp;
	
	struct {
		u32 soft_reset;
		u32 vo_border_color, vo_control, vo_startx, vo_starty, scaler_ctl;
		u32 sb_istnrm, sb_g1crc, sb_g1cwc, sb_g1gdrc, sb_g1gdwc, sb_g2dsto, sb_g2trto, sb_ffst;
		u32 sb_iml2nrm, sb_iml4nrm, sb_iml6nrm;
		u32 sb_iml2ext, sb_iml4ext, sb_iml6ext;
		u32 sb_iml2err, sb_iml4err, sb_iml6err;
		u32 sb_istext, sb_isterr;
		u32 sb_c2dstat, sb_c2dlen, sb_c2dmaxl, sb_c2dpryc, sb_rbsplt;
		
		u32 sb_sdstaw, sb_sdbaaw, sb_sdwlt, sb_sdlas, sb_dbreqm, sb_bavlwc;
		
		u32 sb_mdstar, sb_mdst;
		
		u32 text_control, region_base, param_base;
		u32 y_coeff;
		
		u32 span_sort_cfg;
		u32 fpu_shad_scale, fpu_cull, fpu_param_cfg, half_offset, fpu_perp;
		
		u32 spg_vblank_int, spg_hblank_int, spg_control, spg_hblank, spg_load, spg_width, spg_vblank;
		
		u32 fog_col_ram, fog_col_vert, fog_density, fog_clamp_max, fog_clamp_min;
		u32 fog_table[0x200];
		
		u32 sb_lmmode0, sb_lmmode1;
		
		u32 fb_r_ctrl, fb_x_clip, fb_y_clip, fb_burstctrl, fb_w_sof1, fb_w_sof2, fb_w_ctrl;
		u32 fb_w_linestride, fb_r_sof1, fb_r_sof2, fb_r_size;
		
		u32 unkn68A4;
	} holly;
	
	const u32 HBLANK_IRQ_BIT = 0x20;
	const u32 VBLANK_IN_IRQ_BIT = 0x8;
	const u32 VBLANK_OUT_IRQ_BIT = 0x10;
	const u32 OPAQUE_LIST_CMPL_IRQ_BIT = BIT(7);
	const u32 TRANSP_LIST_CMPL_IRQ_BIT = BIT(9);
	const u32 MAPLE_DMA_CMPL_IRQ_BIT = BIT(12);
};



