#include <print>
#include "dreamcast.h"

u64 dreamcast::io_read(u32 a, u32 sz)
{
	if( debug_on ) { if( a != 0x5F6900 && a != 0x5F6920 ) std::println("read{} <${:X}", sz, a); }
	if( a == 0x5F74EC ) { return 3; /* G1_ATA_BUS_PROTECTION_STATUS_PASSED */ }
	if( a == REGION_BASE_ADDR ) return holly.region_base;
	if( a == HOLLY_ID_ADDR ) return 0x17FD11DB;
	if( a == HOLLY_SOFT_RESET_ADDR ) return holly.soft_reset;
	if( a == VO_BORDER_COLOR_ADDR ) return holly.vo_border_color;
	if( a == VO_CONTROL_ADDR ) return holly.vo_control;
	if( a == SB_ISTNRM_ADDR ) return holly.sb_istnrm;
	if( a == FB_R_CTRL_ADDR ) return holly.fb_r_ctrl;
	if( a == SPAN_SORT_CFG_ADDR ) return holly.span_sort_cfg;
	if( a == FOG_COL_RAM_ADDR ) return holly.fog_col_ram;
	if( a == FOG_COL_VERT_ADDR ) return holly.fog_col_vert;
	if( a == FOG_DENSITY_ADDR ) return holly.fog_density;
	if( a >= 0x5F8200 && a < 0x5F8400 ) { return holly.fog_table[(a-0x5F8200)/4]; }
	if( a == SB_LMMODE0_ADDR ) { return holly.sb_lmmode0; }
	if( a == SB_LMMODE1_ADDR ) { return holly.sb_lmmode1; }
	if( a == TA_OL_BASE_ADDR ) { return ta.ol_base; }
	if( a == TA_OL_LIMIT_ADDR ) { return ta.ol_limit; }
	if( a == TA_ISP_BASE_ADDR ) { return ta.isp_base; }
	if( a == TA_ISP_LIMIT_ADDR ) { return ta.isp_limit; }
	if( a == TA_GLOB_TILE_CLIP_ADDR ) { return ta.glob_tile_clip; }
	if( a == TA_ALLOC_CTRL_ADDR ) { return ta.alloc_ctrl; }
	if( a == TA_NEXT_OPB_INIT_ADDR ) { return ta.next_opb_init; }
	if( a == TA_LIST_INIT_ADDR) { return 0; }
	if( a == FB_Y_CLIP_ADDR ) { return holly.fb_y_clip; }
	if( a == FB_X_CLIP_ADDR ) { return holly.fb_x_clip; }
	if( a == FB_BURSTCTRL_ADDR ) { return holly.fb_burstctrl; }
	if( a == SPG_CONTROL_ADDR ) { return holly.spg_control; }
	if( a == SPG_VBLANK_ADDR ) { return holly.spg_vblank; }
	if( a == SPG_HBLANK_ADDR ) { return holly.spg_hblank; }
	if( a == SPG_LOAD_ADDR ) { return holly.spg_load; }
	if( a == SPG_WIDTH_ADDR ) { return holly.spg_width; }
	if( a == FB_W_SOF1_ADDR ) { return holly.fb_w_sof1; }
	if( a == FB_W_SOF2_ADDR ) { return holly.fb_w_sof2; }
	if( a == FB_W_CTRL_ADDR ) { return holly.fb_w_ctrl; }
	if( a == FB_W_LINESTRIDE_ADDR ) { return holly.fb_w_linestride; }
	if( a == FB_R_SOF1_ADDR ) { return holly.fb_r_sof1; }
	if( a == FB_R_SOF2_ADDR ) { return holly.fb_r_sof2; }
	if( a == FB_R_SIZE_ADDR ) { return holly.fb_r_size; }
	if( a == SPG_HBLANK_INT_ADDR ) { return holly.spg_hblank_int; }
	if( a == FPU_SHAD_SCALE_ADDR ) { return holly.fpu_shad_scale; }
	if( a == FPU_CULL_VAL_ADDR ) { return holly.fpu_cull; }
	if( a == FPU_PARAM_CFG_ADDR ) { return holly.fpu_param_cfg; }
	if( a == HALF_OFFSET_ADDR ) { return holly.half_offset; }
	if( a == FPU_PERP_VAL_ADDR ) { return holly.fpu_perp; }
	if( a == Y_COEFF_ADDR ) { return holly.y_coeff; }
	if( a == ISP_BACKGND_D_ADDR ) { return isp.backgnd_d; }
	if( a == ISP_BACKGND_T_ADDR ) { return isp.backgnd_t; }
	if( a == FOG_CLAMP_MAX_ADDR ) { return holly.fog_clamp_max; }
	if( a == FOG_CLAMP_MIN_ADDR ) { return holly.fog_clamp_min; }
	if( a == TEXT_CONTROL_ADDR ) { return holly.text_control; }
	if( a == VO_STARTX_ADDR ) { return holly.vo_startx; }
	if( a == VO_STARTY_ADDR ) { return holly.vo_starty; }
	if( a == SCALER_CTL_ADDR ) { return holly.scaler_ctl; }
	if( a == SB_G1CRC_ADDR ) { return holly.sb_g1crc; }
	if( a == SB_G1CWC_ADDR ) { return holly.sb_g1cwc; }
	if( a == SB_G1GDWC_ADDR ) { return holly.sb_g1gdrc; }
	if( a == SB_G1GDRC_ADDR ) { return holly.sb_g1gdwc; }
	if( a == SB_FFST_ADDR ) { /*std::println("rd sb_ffst");*/ return 0; return holly.sb_ffst^=0x3f; }
	if( a == SB_IML2NRM_ADDR ) { return holly.sb_iml2nrm; }
	if( a == SB_IML4NRM_ADDR ) { return holly.sb_iml4nrm; }
	if( a == SB_IML6NRM_ADDR ) { return holly.sb_iml6nrm; }
	if( a == SB_IML2EXT_ADDR ) { return holly.sb_iml2ext; }
	if( a == SB_IML4EXT_ADDR ) { return holly.sb_iml4ext; }
	if( a == SB_IML6EXT_ADDR ) { return holly.sb_iml6ext; }
	if( a == SB_IML2ERR_ADDR ) { return holly.sb_iml2err; }
	if( a == SB_IML4ERR_ADDR ) { return holly.sb_iml4err; }
	if( a == SB_IML6ERR_ADDR ) { return holly.sb_iml6err; }
	if( a == SB_ISTEXT_ADDR ) { return holly.sb_istext; }
	if( a == SB_ISTERR_ADDR ) { return holly.sb_isterr; }
	if( a == SB_C2DSTAT_ADDR ) { return holly.sb_c2dstat; }
	if( a == SB_C2DLEN_ADDR ) { return holly.sb_c2dlen; }
	if( a == SB_SDSTAW_ADDR ) { return holly.sb_sdstaw; }
	if( a == SB_SDBAAW_ADDR ) { return holly.sb_sdbaaw; }
	if( a == SB_SDWLT_ADDR ) { return holly.sb_sdwlt; }
	if( a == SB_SDLAS_ADDR ) { return holly.sb_sdlas; }
	if( a == SB_DBREQM_ADDR ) { return holly.sb_dbreqm; }
	if( a == SB_BAVLWC_ADDR ) { return holly.sb_bavlwc; }
	if( a == SB_C2DPRYC_ADDR ) { return holly.sb_c2dpryc; }
	if( a == SB_C2DMAXL_ADDR ) { return holly.sb_c2dmaxl; }
	if( a == SB_RBSPLT_ADDR ) { return holly.sb_rbsplt; }
	if( a == UNKNOWN_REG_68A4 ) { return holly.unkn68A4; }
	if( a == PARAM_BASE_ADDR ) { return holly.param_base; }
	if( a == SB_GDST_ADDR ) { return 0; /* GD DMA never in progress */ }
	if( a == SB_MDST_ADDR ) { return 0; /* Maple DMA never in progress */ }
		
	if( a == SB_ADSUSP_ADDR || a == SB_E1SUSP_ADDR || a == SB_E2SUSP_ADDR || a == SB_DDSUSP_ADDR )
	{
		std::println("Dma status read ${:X}", a);
		return BIT(5)|BIT(4); // dma operations are always possible and have ended
	}	
	if( a == SPG_STATUS_ADDR )
	{
		const u64 cpl = 200000000/60/262;
		u64 line = (stamp - frame_start) / cpl;
		u64 vsync = (line >= 240 ? BIT(13) : 0);
		return vsync | line;
	}
	
	if( a >= 0x5F7000 && a < 0x5F7100 ) return gdread(a, sz);
	
	if( a == 0x5F8138 ) return 0;
	
	std::println("Unimpl IO Read{} ${:X}", sz, a);
	exit(1);
}

void dreamcast::io_write(u32 a, u64 v, u32 sz)
{
	if( debug_on ) { if( a != 0x5F6900 
			  && a != 0x5F6920 
			  && a != 0x5F6C04 
			  && a != 0x5F6C18 ) std::println("write{} ${:X}=${:X}", sz, a, v); }
	if( a == STARTRENDER_ADDR ) { start_render(); return; }
	if( a == PARAM_BASE_ADDR ) { holly.param_base = v; return; }
	if( a == REGION_BASE_ADDR ) { holly.region_base = v & 0xffFFfc; return; }
	if( a == HOLLY_SOFT_RESET_ADDR ) { holly.soft_reset = v&7; return; }
	if( a == VO_BORDER_COLOR_ADDR ) { holly.vo_border_color = v&0x1ffFFff; return; }
	if( a == VO_CONTROL_ADDR ) { holly.vo_control = v; return; }
	if( a == SB_ISTNRM_ADDR ) { holly.sb_istnrm = (holly.sb_istnrm&(BIT(31)|BIT(30))) | (holly.sb_istnrm&~v); return; }
	if( a == FB_R_CTRL_ADDR ) { holly.fb_r_ctrl = v; return; }
	if( a == SPAN_SORT_CFG_ADDR ) { holly.span_sort_cfg = v & 0x10101; return; }
	if( a == FOG_COL_RAM_ADDR ) { holly.fog_col_ram = v&0xffFFff; return; }
	if( a == FOG_COL_VERT_ADDR ) { holly.fog_col_vert = v&0xffFFff; return; }
	if( a == FOG_DENSITY_ADDR ) { holly.fog_density = v&0xffff; return; }
	if( a >= 0x5F8200 && a < 0x5F8400 ) { holly.fog_table[(a-0x5F8200)/4] = v&0xffff; return; }
	if( a == SB_LMMODE0_ADDR ) { holly.sb_lmmode0 = v&1; return; }
	if( a == SB_LMMODE1_ADDR ) { holly.sb_lmmode1 = v&1; return; }
	if( a == SPG_VBLANK_INT_ADDR ) { holly.spg_vblank_int = v; std::println("vin = {}, vout={}",v&0x3ff,(v>>16)&0x3ff); return; }
	if( a == TA_OL_BASE_ADDR ) { ta.ol_base = v&0xffFFff; return; }
	if( a == TA_OL_LIMIT_ADDR ) { ta.ol_limit = v&0xffFFff; return; }
	if( a == TA_ISP_BASE_ADDR ) { ta.isp_base = v&0xffFFff; return; }
	if( a == TA_ISP_LIMIT_ADDR ) { ta.isp_limit = v&0xffFFff; return; }
	if( a == TA_GLOB_TILE_CLIP_ADDR ) { ta.glob_tile_clip = v; return; }
	if( a == TA_ALLOC_CTRL_ADDR ) { ta.alloc_ctrl = v; return; }
	if( a == TA_NEXT_OPB_INIT_ADDR ) { ta.next_opb_init = v; return; }
	if( a == TA_LIST_INIT_ADDR ) { if( v & BIT(31) ) ta_list_init(); return; }
	if( a == FB_Y_CLIP_ADDR ) { holly.fb_y_clip = v; return; }
	if( a == FB_X_CLIP_ADDR ) { holly.fb_x_clip = v; return; }
	if( a == FB_BURSTCTRL_ADDR ) { holly.fb_burstctrl = v; return; }
	if( a == SPG_CONTROL_ADDR ) { holly.spg_control = v; return; }
	if( a == SPG_VBLANK_ADDR ) { holly.spg_vblank = v; return; }
	if( a == SPG_HBLANK_ADDR ) { holly.spg_hblank = v; return; }
	if( a == SPG_LOAD_ADDR ) { holly.spg_load = v; return; }
	if( a == SPG_WIDTH_ADDR ) { holly.spg_width = v; return; }
	if( a == FB_W_SOF1_ADDR ) { holly.fb_w_sof1 = v&0x1ffFFfc; return; }
	if( a == FB_W_SOF2_ADDR ) { holly.fb_w_sof2 = v&0x1ffFFfc; return; }
	if( a == FB_W_CTRL_ADDR ) { holly.fb_w_ctrl = v; return; }
	if( a == FB_W_LINESTRIDE_ADDR ) { holly.fb_w_linestride = v; return; }
	if( a == FB_R_SOF1_ADDR ) { holly.fb_r_sof1 = v&0x1ffFFfc; return; }
	if( a == FB_R_SOF2_ADDR ) { holly.fb_r_sof2 = v&0x1ffFFfc; return; }
	if( a == FB_R_SIZE_ADDR ) { holly.fb_r_size = v; return; }
	if( a == SPG_HBLANK_INT_ADDR ) { std::println(stderr, "hblank_int = ${:X}", v); holly.spg_hblank_int = v; return; }
	if( a == FPU_SHAD_SCALE_ADDR ) { holly.fpu_shad_scale = v&0x1ff; return; }
	if( a == FPU_CULL_VAL_ADDR ) { holly.fpu_cull = v; return; }
	if( a == FPU_PARAM_CFG_ADDR ) { holly.fpu_param_cfg = v; return; }
	if( a == HALF_OFFSET_ADDR ) { holly.half_offset = v&7; return; }
	if( a == FPU_PERP_VAL_ADDR ) { holly.fpu_perp = v; return; }
	if( a == Y_COEFF_ADDR ) { holly.y_coeff = v&0xffff; return; }
	if( a == ISP_BACKGND_D_ADDR ) { isp.backgnd_d = v; return; }
	if( a == ISP_BACKGND_T_ADDR ) { isp.backgnd_t = v; return; }
	if( a == FOG_CLAMP_MAX_ADDR ) { holly.fog_clamp_max = v; return; }
	if( a == FOG_CLAMP_MIN_ADDR ) { holly.fog_clamp_min = v; return; }
	if( a == TEXT_CONTROL_ADDR ) { holly.text_control = v; return; }
	if( a == VO_STARTX_ADDR ) { holly.vo_startx = v; return; }
	if( a == VO_STARTY_ADDR ) { holly.vo_starty = v; return; }
	if( a == SCALER_CTL_ADDR ) { holly.scaler_ctl = v; return; }
	if( a == SB_G1CRC_ADDR ) { holly.sb_g1crc = 0xFF7&v; return; }
	if( a == SB_G1CWC_ADDR ) { holly.sb_g1cwc = 0xFF7&v; return; }
	if( a == SB_G1GDRC_ADDR ) { holly.sb_g1gdrc = 0xFFff&v; return; }
	if( a == SB_G1GDWC_ADDR ) { holly.sb_g1gdwc = 0xFFff&v; return; }
	if( a == SB_G2TRTO_ADDR ) { holly.sb_g2trto = 0xfff&v; return; }
	if( a == SB_G2DSTO_ADDR ) { holly.sb_g2dsto = 0xfff&v; return; }
	if( a == SB_IML2NRM_ADDR ) { holly.sb_iml2nrm = v; return; }
	if( a == SB_IML4NRM_ADDR ) { holly.sb_iml4nrm = v; return; }
	if( a == SB_IML6NRM_ADDR ) { holly.sb_iml6nrm = v; return; }
	if( a == SB_IML2EXT_ADDR ) { holly.sb_iml2ext = v; return; }
	if( a == SB_IML4EXT_ADDR ) { holly.sb_iml4ext = v; return; }
	if( a == SB_IML6EXT_ADDR ) { holly.sb_iml6ext = v; return; }
	if( a == SB_IML2ERR_ADDR ) { holly.sb_iml2err = v; return; }
	if( a == SB_IML4ERR_ADDR ) { holly.sb_iml4err = v; return; }
	if( a == SB_IML6ERR_ADDR ) { holly.sb_iml6err = v; return; }
	if( a == SB_ISTEXT_ADDR ) { std::println("sb_istext ro but wr = ${:X}", v); return; }
	if( a == SB_ISTERR_ADDR ) { holly.sb_isterr &= ~v; return; }
	if( a == SB_C2DSTAT_ADDR ) { holly.sb_c2dstat = (v ? v : BIT(28)); return; }
	if( a == SB_C2DLEN_ADDR ) { holly.sb_c2dlen = v; return; }
	if( a == SB_SDSTAW_ADDR ) { holly.sb_sdstaw = v; return; }
	if( a == SB_SDBAAW_ADDR ) { holly.sb_sdbaaw = v; return; }
	if( a == SB_SDWLT_ADDR ) { holly.sb_sdwlt = v&1; return; }
	if( a == SB_SDLAS_ADDR ) { holly.sb_sdlas = v&1; return; }
	if( a == SB_DBREQM_ADDR ) { holly.sb_dbreqm = v&1; return; }
	if( a == SB_BAVLWC_ADDR ) { holly.sb_bavlwc = v&0x1f; return; }
	if( a == SB_C2DPRYC_ADDR ) { holly.sb_c2dpryc = v&0xf; return; }
	if( a == SB_C2DMAXL_ADDR ) { holly.sb_c2dmaxl = v&3; return; }
	if( a == SB_RBSPLT_ADDR ) { holly.sb_rbsplt = v&BIT(31); return; }
	if( a == UNKNOWN_REG_68A4 ) { holly.unkn68A4 = v; return; }
	if( a >= 0x5F68A0 && a < 0x5F68B0 ) { std::println("Unkn reg write{} ${:X} = ${:X}", sz, a, v); return; }
	if( a == 0x5F74E4 ) { return; /*GD_UNLOCK*/ }
	if( a == SB_MDSTAR_ADDR ) { holly.sb_mdstar = v & 0x1fffFFe0; return; }
	if( a == SB_MDST_ADDR ) { if( v&1 ) maple_dma(); return; }
	
	
	if( a == SB_ADSUSP_ADDR || a == SB_E1SUSP_ADDR || a == SB_E2SUSP_ADDR || a == SB_DDSUSP_ADDR )
	{
		return; // only writable bit is write-only
	}

	if( a >= 0x5F7000 && a < 0x5F7100 ) return gdwrite(a, v, sz);

	std::println("Unimpl IO Write{} ${:X} = ${:X}", sz, a, v);
	//exit(1);
}


u64 dreamcast::read(u32 a, u32 sz)
{
	if( (a & 0xf0000000u) == 0xf0000000u ) { return cpureg_read(a, sz); }
	if( (a & 0xf0000000u) == 0xe0000000u )
	{
		//todo: store queue
		return 0;
	}
	
	a &= 0x1fffFFFFu;
	
	if( a < 0x200000 ) return sized_read(bios, a, sz);
	if( a < 0x220000 ) return sized_read(flashrom, a&0x1ffff, sz);
	
	if( a >= 0x5F6000 && a < 0x5FA000 ) { return io_read(a, sz); }
	if( a == 0x600004 ) { return 0xff; /* modem stuff? */ }
		
	if( a >= 0x700000 && a < 0x800000 ) { return 0; /* snd ctrl todo */ }
	
	if( a >= 0x800000 && a < 0xa00000 ) return sized_read(sndram, a&0x1fffff, sz);	
	if( a >= 0xc000000 && a < 0x10000000 ) return sized_read(RAM, a&0xffFFff, sz);

	if( a >= 0x4000000 && a < 0x4800000 ) 
	{
		a &= 0x7fffff;
		a = (((a&0x3fffff)>>1)&~3)|((a&BIT(2))?0x400000:0)|(a&3);
		return sized_read(vram, a&0x7fffff, sz);
	}
	if( a >= 0x5000000 && a < 0x5800000 ) return sized_read(vram, a&0x7fffff, sz);
	
	if( a >= 0x1E000000 && a < 0x1E001fff ) return (intern.CCR&CCR_ORA_BIT) ? sized_read(opcache,a&0x1fff,sz):0;
	std::println("DC: Unimpl read{} <${:X}", sz, a);
	exit(1);
}

void dreamcast::write(u32 a, u64 v, u32 sz)
{
	if( (a & 0xf0000000u) == 0xf0000000u ) { cpureg_write(a, v, sz); return; }
	if( (a & 0xf0000000u) == 0xe0000000u )
	{
		storeq[(a&0x3f)>>2] = v;
		if( sz == 64 ) { a+=4; storeq[(a&0x3f)>>2] = v>>32; }
		return;
	}
	
	a &= 0x1fffFFFFu;
	
	if( a < 0x220000 ) return;
	
	if( a >= 0x800000 && a < 0xa00000 ) return sized_write(sndram, a&0x1fffff, v, sz);	
	if( a >= 0xc000000 && a < 0x10000000 ) return sized_write(RAM, a&0xffFFff, v, sz);

	if( a >= 0x5F6000 && a < 0x5FA000 ) { return io_write(a, v, sz); }

	if( a >= 0x4000000 && a < 0x4800000 ) 
	{
		a &= 0x7fffff;
		a = (((a&0x3fffff)>>1)&~3)|((a&BIT(2))?0x400000:0)|(a&3);
		sized_write(vram, a&0x7fffff, v, sz);
		return;
	}
	
	if( a >= 0x5000000 && a < 0x5800000 ) return sized_write(vram, a&0x7fffff, v, sz);

	if( a >= 0x1E000000 && a < 0x1E001fff ) 
	{ 
		if(intern.CCR&CCR_ORA_BIT) sized_write(opcache,a&0x1fff,v,sz); 
		return;
	}
	
	if( a >= 0x10000000 && a <= 0x13FFFFFF )
	{
		if( sz < 32 ) { std::println("TA write under 32bits ignored"); return; }
		ta_input(v);
		if( sz == 64 ) ta_input(v>>32);
		return;
	}

	if( a >= 0x700000 && a < 0x800000 ) { return; /* snd ctrl todo */ }
	
	std::println("DC: Unimpl write{} ${:X} = ${:X}", sz, a, v);
	exit(1);
}

u32 dreamcast::fb_width()
{
	return 640;
}

u32 dreamcast::fb_height()
{
	return 480;
}

u32 dreamcast::fb_scale_w()
{
	return 640;
}

u32 dreamcast::fb_scale_h()
{
	return 480;
}

u32 dreamcast::fb_bpp()
{
	static u32 bpplist[] = { 16, 17, 32, 33 };  
	// will need demos that actually use modes other than 33. the 256/128b demos don't properly set FB_R_CTRL
	return bpplist[(holly.fb_r_ctrl>>2)&3];
}

void dreamcast::check_irqs()
{
	u8 lvl = 0;
	u16 evt = 0;
	bool fire = false;
	
	if( ((holly.sb_istnrm & holly.sb_iml6nrm & 0x3fffff)||(holly.sb_istext & holly.sb_iml6ext & 15)) )
	{
		lvl = 6;
		evt = 0x320;
		fire = true;
	} else if( ((holly.sb_istnrm & holly.sb_iml4nrm & 0x3fffff)||(holly.sb_istext & holly.sb_iml4ext & 15)) ) {
		lvl = 4;
		evt = 0x360;
		fire = true;
	} else if( ((holly.sb_istnrm & holly.sb_iml2nrm & 0x3fffff)||(holly.sb_istext & holly.sb_iml2ext & 15)) ) {
		lvl = 2;
		evt = 0x3A0;
		fire = true;
	} 
	
	const u8 t0_lvl = (intern.IPRA>>12)&15;
	const u8 t1_lvl = (intern.IPRA>>8)&15;
	const u8 t2_lvl = (intern.IPRA>>4)&15;
	
	if( lvl < t0_lvl && (intern.TCR0 & TMR_IRQ_ACTIVE) == TMR_IRQ_ACTIVE ) {
		lvl = t0_lvl;
		evt = 0x400;
		fire = true;
	}
	if( lvl < t1_lvl && (intern.TCR1 & TMR_IRQ_ACTIVE) == TMR_IRQ_ACTIVE ) {
		lvl = t1_lvl;
		evt = 0x420;
		fire = true;
	} 
	if( lvl < t2_lvl && (intern.TCR2 & TMR_IRQ_ACTIVE) == TMR_IRQ_ACTIVE ) {
		lvl = t2_lvl;
		evt = 0x440;
		fire = true;
	}
	
	if( fire && cpu.sr.b.BL == 1 && !cpu.sleeping ) { /*if( debug_on ) std::println("IRQs blocked");*/ return; }
	if( !fire || (lvl <= cpu.sr.b.IMASK) ) return;
	//std::println("got here with lvl = ${:X}, env = ${:X}", lvl, env);
	//std::println("IMASK = ${:X}", (u32) cpu.sr.b.IMASK);
	
	cpu.SGR = cpu.r[15];
	cpu.SPC = cpu.pc;
	cpu.SSR = cpu.sr.v;
	
	auto sr = cpu.sr;
	sr.b.MD = 1;
	sr.b.BL = 1;
	sr.b.RB = 1;
	cpu.setSR(sr.v);
	cpu.INTEVT = evt;
	cpu.pc = cpu.VBR + 0x600;
	cpu.sleeping = false;
	//std::println("IRQ to ${:X}", cpu.pc);
}

void dreamcast::run_frame()
{
	frame_start = stamp;
	sched.add_event(stamp + ((200000000/60)/263)*(holly.spg_hblank_int&0x3ff), HBLANK_IN);
	sched.add_event(stamp + ((200000000/60)/263)*((holly.spg_vblank_int>>16)&0x3ff), VBLANK_OUT);
	sched.add_event(stamp + ((200000000/60)/263)*((holly.spg_vblank_int&0x3ff)), VBLANK_IN);
	for(u32 i = 0; i < (200000000/60); ++i)
	{
		//if( debug_on && cpu.pc > 0x8c010000u ) std::println("pc = ${:X}", cpu.pc);
		check_irqs();
		if( !cpu.sleeping ) cpu.step();
		if( (cpu.pc>>8) == 0xcafeba )
		{
			u32 bcall = cpu.pc;
			if( (bcall&0xff) == 0xe0 )
			{
				cpu.sr.b.IMASK = 0;
				holly.sb_iml2nrm = 0;
			} else if( (bcall&0xff) == 0xb4  ) { // romfont
				if( cpu.r[1] == 0 ) cpu.r[0] = 0xA0100020;
				else if( cpu.r[1] == 1 ) cpu.r[0] = 0;
			} else if( (bcall&0xff) == 0xbc ) {
				if( cpu.r[7] == 4 )
				{
					u32 addr = cpu.r[4] & 0xffFFff;
					*(u32*)&RAM[addr] = 2;
					*(u32*)&RAM[addr+4] = 0x20;
					cpu.r[0] = 0;
				} else if( cpu.r[7] == 1 ) {
					cpu.r[0] = 2;
					u32 addr = cpu.r[5] & 0xffFFff;
					for(u32 x=0;x<4;++x) *(u32*)&RAM[addr+x*4]=0;
				}
				std::println("${:X}: BIOS call:${:X}: r4=${:X}, r6=${:X}, r7=${:X}", 
					cpu.pc, bcall&0xff, cpu.r[4], cpu.r[6], cpu.r[7]);
			} else {
				//cpu.r[0] = 0;
				std::println("${:X}: BIOS call:${:X}: r4=${:X}, r6=${:X}, r7=${:X}", 
					cpu.pc, bcall&0xff, cpu.r[4], cpu.r[6], cpu.r[7]);
			}
			cpu.pc = cpu.PR;	
		}
		stamp += 1;
		while( stamp >= sched.next_stamp() )
		{
			sched.run_event();
		}
	}
	
	
}

void dreamcast::reset()
{
	//cpu.pc = 0x8c008300u;
	memset(&holly, 0, sizeof(holly));
	memset(&ta, 0, sizeof(ta));
	memset(&intern, 0, sizeof(intern));
	memset(&gd, 0, sizeof(gd));
	
	holly.vo_control = 0;
	holly.fb_r_ctrl = 0xc;
	holly.fb_r_sof1 = 0x200000;
	//holly.sb_istnrm |= 0x80;
	cpu.sr.b.IMASK = 15;
	intern.IPRA = intern.IPRB = intern.IPRC = 0;
	
	intern.TSTR = 0;
	intern.TCR0 = intern.TCR1 = intern.TCR2 = 0;
	intern.TCNT0 = intern.TCNT1 = intern.TCNT2 = 0xffffFFFFu;
	intern.TCOR0 = intern.TCOR1 = intern.TCOR2 = 0xffffFFFFu;
	debug_on = false;
}

bool dreamcast::loadROM(std::string fname)
{
	cpu.bus_read = [&](u32 a, u32 sz)->u64 { return read(a,sz); };
	cpu.bus_write =[&](u32 a, u64 v, u32 sz) { write(a,v,sz); };
	cpu.fetch = [&](u32 a)->u16 { return read(a,16); };
	cpu.pref = [&](u32 a) { write_storeQ(a); };
	cpu.init();

	if( !freadall(bios, fopen("./bios/dc_boot.bin", "rb"), 2_MB) )
	{
		std::println("Unable to open './bios/dc_boot.bin'");
		return false;	
	}
	
	if( !freadall(flashrom, fopen("./bios/dc_flash.bin", "rb"), 128_KB) )
	{
		std::println("Unable to open './bios/dc_flash.bin'");
		return false;
	}
	
	for (u32 i = 0; i < 16; i++) 
	{
		cpu.write(0x8C0000E0 + 2 * i, cpu.read(0x000000FE - 2 * i, 16), 16);
	}

	memcpy(&RAM[0x100], bios + 0x100, 0x3F00);
	//memcpy(&RAM[0x8000], bios + 0x8000, 0x1F800);

	cpu.write( 0x8C0000B0, 0x8C003C00, 32);
	cpu.write( 0x8C0000B4, 0x8C003D80, 32);
	cpu.write( 0x8C0000B8, 0x8C003D00, 32);
	cpu.write( 0x8C0000BC, 0x8C001000, 32);
	cpu.write( 0x8C0000C0, 0x8C0010F0, 32);
	cpu.write( 0x8C0000E0, 0x8C000800, 32);

	cpu.write( 0x8C0000AC, 0xA05F7000, 32);
	cpu.write( 0x8C0000A8, 0xA0200000, 32);
	cpu.write( 0x8C0000A4, 0xA0100000, 32);
	cpu.write( 0x8C0000A0, 0, 32);
	cpu.write( 0x8C00002C, 0, 32);
	cpu.write( 0x8CFFFFF8, 0x8C000128, 32);
	
	cpu.write( 0xAC0090Db, 0x5113, 16);
	cpu.write( 0xAC00940A, 0xB, 16);
	cpu.write( 0xAC00940C, 0x9, 16);

	cpu.write( 0x8C000000, 0x00090009, 32);
	cpu.write( 0x8C000004, 0x001B0009, 32);
	cpu.write( 0x8C000008, 0x0009AFFD, 32);

	cpu.write( 0x8C00000C, 0, 16);
	cpu.write( 0x8C00000E, 0, 16);

	cpu.write( 0x8C000010, 0x00090009, 32);
	cpu.write( 0x8C000014, 0x0009002B, 32);

	cpu.write( 0x8C000018, 0x00090009, 32);
	cpu.write( 0x8C00001C, 0x0009000B, 32);

	cpu.write( 0x8C00002C, 0x16, 8);
	cpu.write( 0x8C000064, 0x8C008100, 32);
	cpu.write( 0x8C000090, 0, 16);
	cpu.write( 0x8C000092, -128, 16);
	
	
	cpu.sr.v = 0x600000f0;
	cpu.fpctrl.v = 0x00040001;

	cpu.r[0] = 0x8c010000; //0xAC0005D8;
	cpu.r[1] = 0x00000808; // 0x00000009;
	cpu.r[2] = 0x8c00e070; // 0xAC00940C;
	cpu.r[3] = 0x8c010000; // 0;
	cpu.r[4] = 0x8c010000; //0xAC008300;
	cpu.r[5] = 0xF4000000;
	cpu.r[6] = 0xF4002000;
	cpu.r[7] = 0x00000044;
	cpu.r[8] = 0;
	cpu.r[9] = 0;
	cpu.r[10] = 0;
	cpu.r[11] = 0;
	cpu.r[12] = 0;
	cpu.r[13] = 0;
	cpu.r[14] = 0;
	cpu.r[15] = 0x8c00f400;
	
	cpu.GBR = 0x8C000000;
	//cpu.SSR = 0x40000001;
	//cpu.SPC = 0x8C000776;
	//cpu.SGR = 0x8D000000;
	//cpu.DBR = 0x8C000010;
	cpu.VBR = 0x8c000000; //0x8C000000;
	cpu.PR = 0x8c00e09c;//0x0C00043C;
	cpu.FPUL = 0;
	cpu.pc = 0x8c008300u; //0xa0000000u;// 
	stamp = 0;
	
	holly.vo_control = 0x108;
	for(u32 i = 0xb0; i < 0xf0; i+=4) *(u32*)&RAM[i] = 0xcafeba00|i;
	
	if( !freadall(RAM+0x8000, fopen("./bios/dc_IP.BIN", "rb")) )
	{
		std::println("Unable to open './bios/dc_IP.BIN'");
		return false;	
	}
		
	if( !freadall(RAM+0x10000, fopen(fname.c_str(), "rb")) )
	{
		std::println("Unable to open dreamcast program '{}'", fname);
		return false;
	}
	
	return true;
}

void dreamcast::event(u64 oldstamp, u32 code)
{
	if( code == TMR0_UNDERFLOW )
	{
		timer_underflow(oldstamp, 0);
		return;
	}

	if( code == TMR1_UNDERFLOW )
	{
		timer_underflow(oldstamp, 1);
		return;
	}

	if( code == TMR2_UNDERFLOW )
	{
		timer_underflow(oldstamp, 2);
		return;
	}
	
	if( code == VBLANK_IN )
	{
		holly.sb_istnrm |= 8;
		memcpy(fbuf, vram+(holly.fb_r_sof1&0x7ffff0), 640*480*4);
		ta_clear = true;
		return;
	}
	
	if( code == VBLANK_OUT )
	{
		holly.sb_istnrm |= 0x10;
		return;
	}
	
	if( code == HBLANK_IN )
	{
		holly.sb_istnrm |= 0x20;
		return;
	}
}

void dreamcast::render_opaque(u32 objlist)
{
	u32 data = *(u32*)&vram[objlist];
	if( (data>>28) != 0xf ) std::println("opaqlist @${:X}: startswith ${:X}", objlist, data);
}

void dreamcast::start_render()
{
	static const u32 obj_sizes[] = { 0, 8, 16, 32 };
	u32 rbase = holly.region_base;
	u32 rasize = (holly.fpu_param_cfg&BIT(21)) ? 6 : 5;
	u32 tiles_w = 1 + (ta.glob_tile_clip & 0x3f);
	u32 tiles_h = 1 + ((ta.glob_tile_clip>>16) & 15);
	u32 obj = ta.next_opb_init;
	u32 opaque_size = obj_sizes[ta.alloc_ctrl&3];
	
	u32* region = nullptr;	
	for(u32 x = 0; x < tiles_w; ++x)
	{
		for(u32 y = 0; y < tiles_h; ++y)
		{
			region = (u32*)&vram[rbase];
			rbase += rasize*4;
			if( !(region[1] & BIT(31)) ) render_opaque(region[1]);
			if( region[0] & BIT(31) ) break;
		}
	}
	
	std::println(stderr,"opaq irq");
	holly.sb_istnrm |= 7;
}

void dreamcast::ta_list_init()
{
	static const u32 obj_sizes[] = { 0, 8, 16, 32 };
	u32 rbase = holly.region_base;
	u32 rasize = (holly.fpu_param_cfg&BIT(21)) ? 6 : 5;
	u32 tiles_w = 1 + (ta.glob_tile_clip & 0x3f);
	u32 tiles_h = 1 + ((ta.glob_tile_clip>>16) & 15);
	u32 obj = ta.next_opb_init;
	u32 opaque_size = obj_sizes[ta.alloc_ctrl&3];
	
	u32* region = nullptr;	
	for(u32 x = 0; x < tiles_w; ++x)
	{
		for(u32 y = 0; y < tiles_h; ++y)
		{
			region = (u32*)&vram[rbase];
			rbase += rasize*4;
			memset(region, 0, rasize*4);
			region[0] |= (x<<2)|(y<<8);
			for(u32 i = 1; i < rasize; ++i) { region[i] |= BIT(31); }
			region[1] = obj;
			*(u32*)&vram[obj] = 0xf0000000u;
			obj += opaque_size*4;
		}
	}
	
	region[0] |= BIT(31);
}

void dreamcast::maple_dma()
{
	u32 start = holly.sb_mdstar & 0xffFFff;
	//std::println("Maple DMA Initiated from ${:X}", start);
	
	bool dmadone = false;
	u32 temp = start;
	while( !dmadone )
	{
		u32 header = *(u32*)&RAM[temp];
		if( header & BIT(31) ) dmadone = true;
		u32 resaddr = *(u32*)&RAM[temp+4] & 0xffFFff;
		temp += 8;
		u32 words = (header&0x7f)+1;
		//std::println("maple dsc ${:X}, ${:X}", header, resaddr);		

		u32* frameptr = (u32*)&RAM[temp];
		const u32 port = ((header>>16)&3);
		switch( port )
		{
		case 0: maple_port1(resaddr, frameptr, words); break;
		case 1: maple_port2(resaddr, frameptr, words); break;
		default:
			*(u32*)&RAM[resaddr] = 0xffffFFFFu; break;
		}
		temp += words*4;
	}
	
	holly.sb_istnrm |= MAPLE_DMA_CMPL_IRQ_BIT;// BIT(12);
}

void dreamcast::write_storeQ(u32 pref)
{
	u32 ptr = pref & 0x03FFFFE0;
	if( pref & BIT(5) )
	{
		ptr |= (intern.QACR1<<24);
	} else {
		ptr |= (intern.QACR1<<24);
	}
	for(u32 i = 0; i < 8; ++i)
	{
		write(ptr+i*4, storeq[i + ((pref&BIT(5))?8:0)], 32);
	}
}


