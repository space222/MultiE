#pragma once
#include "console.h"
#include "VR4300.h"
#include "n64_rsp.h"
#include "n64_rdp.h"

class n64 : public console
{
public:
	n64() { setVsync(0); }
	~n64();
	u32 fb_width() override { return curwidth; }
	u32 fb_height() override { return curheight; }
	u32 fb_bpp() override { return curbpp; }
	u32 fb_scale_w() override { return 640; }
	u32 fb_scale_h() override { return 480; }
	u8* framebuffer() override { return &fbuf[0]; }
	
	bool loadROM(const std::string) override;
	void run_frame() override;
	void reset() override;
	
	u64 read(u32, int);
	void write(u32, u64, int);
	
	VR4300 cpu;
	
	u32 curwidth, curheight, curbpp;

	std::vector<u8> mem;
	std::vector<u8> ROM;
	u8 fbuf[640*480*4];
	
	u32 vi_regs[0x10];
	u32& VI_CTRL = vi_regs[0];
	u32& VI_ORIGIN = vi_regs[1];
	u32& VI_WIDTH = vi_regs[2];
	u32& VI_V_INTR = vi_regs[3];
	u32& VI_V_CURRENT = vi_regs[4];
	u32& VI_BURST = vi_regs[5];
	u32& VI_V_TOTAL = vi_regs[6];
	u32& VI_H_TOTAL = vi_regs[7];
	u32& VI_H_TOTAL_LEAP = vi_regs[8];
	u32& VI_H_VIDEO = vi_regs[9];
	u32& VI_V_VIDEO = vi_regs[10];
	u32& VI_V_BURST = vi_regs[11];
	u32& VI_X_SCALE = vi_regs[12];
	u32& VI_Y_SCALE = vi_regs[13];
	void vi_draw_frame();
	void vi_write(u32, u32);
	u32 vi_read(u32);
	
	u32 pi_regs[0x10];
	u32& PI_DRAM_ADDR = pi_regs[0];
	u32& PI_CART_ADDR = pi_regs[1];
	u32& PI_RD_LEN = pi_regs[2];
	u32& PI_WR_LEN = pi_regs[3];
	u32& PI_STATUS = pi_regs[4];
	u32& PI_BSD_DOM1_LAT = pi_regs[5];
	u32& PI_BSD_DOM1_PWD = pi_regs[6];
	u32& PI_BSD_DOM1_PGS = pi_regs[7];
	u32& PI_BSD_DOM1_RLS = pi_regs[8];
	u32& PI_BSD_DOM2_LAT = pi_regs[9];
	u32& PI_BSD_DOM2_PWD = pi_regs[10];
	u32& PI_BSD_DOM2_PGS = pi_regs[11];
	u32& PI_BSD_DOM2_RLS = pi_regs[12];
	void pi_write(u32, u32);
	u32 pi_read(u32);
	void pi_dma(bool);
	s64 pi_cycles_til_irq;
	
	bool pif_rom_enabled, do_boot_hle;
	u8 pifrom[0x800];
	u8 pifram[0x40];
	
	const u32 MI_INTR_DP_BIT = 5;
	const u32 MI_INTR_PI_BIT = 4;
	const u32 MI_INTR_VI_BIT = 3;
	const u32 MI_INTR_AI_BIT = 2;
	const u32 MI_INTR_SI_BIT = 1;
	const u32 MI_INTR_SP_BIT = 0;
	void raise_mi_bit(u32 b);
	void clear_mi_bit(u32 b);
	
	u32 mi_regs[4];
	u32& MI_MODE = mi_regs[0];
	u32& MI_VERSION = mi_regs[1];
	u32& MI_INTERRUPT = mi_regs[2];
	u32& MI_MASK = mi_regs[3];
	void mi_write(u32, u32);
	u32 mi_read(u32);
	
	struct {
		u32 ramaddr;
		u32 length;
		bool valid;
	} ai_buf[2];
	u64 ai_cycles_per_sample, ai_cycles;
	u64 ai_output_cycles;
	float ai_L, ai_R;
	bool ai_dma_enabled;
	u32 ai_read(u32);
	void ai_write(u32, u32);
	
	u32 si_regs[7];
	u32& SI_STATUS = si_regs[6];
	u32 si_read(u32);
	void si_write(u32, u32);
	void pif_run();
	u64 si_cycles_til_irq;
	
	u8 DMEM[0x1000];
	u8 IMEM[0x1000];
	u32 sp_regs[8];
	u32& SP_DMA_SPADDR = sp_regs[0];
	u32& SP_DMA_RAMADDR = sp_regs[1];
	u32& SP_DMA_RDLEN = sp_regs[2];
	u32& SP_DMA_WRLEN = sp_regs[3];
	u32& SP_STATUS = sp_regs[4];
	u32& SP_SEMA = sp_regs[7];
	u32 sp_read(u32);
	void sp_write(u32, u32);
	void sp_read_dma();
	void sp_write_dma();
	n64_rsp RSP;
	u32 rspdiv;
	
	u32 dp_regs[8];
	u32& DP_START = dp_regs[0];
	u32& DP_END = dp_regs[1];
	u32& DP_CURRENT = dp_regs[2];
	u32& DP_STATUS = dp_regs[3];
	u32 dp_read(u32);
	void dp_write(u32, u32);
	void dp_send();
	n64_rdp RDP;
	struct {
		u32 start, end, current;
		bool valid;
	} dp_xfer;
	
	u8 eeprom[2048];
	bool eeprom_written;
	std::string save_file;
};

