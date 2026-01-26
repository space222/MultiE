#pragma once
#include <print>
#include <deque>
#include "console.h"
#include "EEngine.h"
#include "r3000.h"

extern bool logall;

class ps2 : public console
{
public:
	u32 fb_width() override { return 640; }
	u32 fb_height() override { return 480; }
	u32 fb_bpp() override { return 32; }
	
	u8* framebuffer() override { return (u8*)fbuf; }
	
	void run_frame() override;
	bool loadROM(std::string) override;
	void reset() override;
	
	void key_down(int f)
	{ 
		if( f == SDL_SCANCODE_ESCAPE ) logall = !logall; 
		if( f == SDL_SCANCODE_F )
		{
			std::println("ee INTC_MASK = ${:X}", eeint.INTC_MASK);
			
		}
	}
	
	//bool logall = false;
	
	void iop_write(u32 a, u32 v, int sz);
	u32 iop_read(u32 a, int sz);
	
	u128 read(u32, int);
	void write(u32, u128, int);
	
	u64 global_stamp=0, iop_stamp=0;
	u64 last_target=0;

	EEngine cpu;
	r3000 iop;

	u8 BIOS[4_MB];
	u8 spad[16_KB];
	u8 RAM[32_MB];
	u8 vram[4_MB];
	
	u8 iop_spad[1_KB];
	u8 iop_ram[2_MB];
	
	u32 fbuf[640*480];
	
	struct {
		u32 INTC_STAT=0;
		u32 INTC_MASK=0;
	} eeint;
	
	struct {
		u32 D_CTRL=0, D_PCR=0, D_SQWC=0;
		union {
			struct {
				unsigned int stat : 10;
				unsigned int pad0 : 3;
				unsigned int dma_stall : 1;
				unsigned int mfifo_empty : 1;
				unsigned int buserr : 1;
				unsigned int mask : 10;
				unsigned int pad1 : 3;
				unsigned int dma_stall_mask : 1;
				unsigned int mfifo_empty_mask : 1;
				unsigned int pad2 : 1;
			} PACKED b;
			u32 v;
		} PACKED D_STAT = {.v=0};
	
		u32 chan[10][15] = {0};
		
		u32 D_RBSR=0, D_RBOR=0;
		
		std::deque<u32> sif_fifo;
		u32 last_fifo=0;
		u32 pop_fifo() { if( !sif_fifo.empty() ) { last_fifo = sif_fifo.back(); sif_fifo.pop_back(); } return last_fifo; }

		u128 popq()
		{
			u128 v = pop_fifo();
			v |= u128(pop_fifo())<<32;
			v |= u128(pop_fifo())<<64;
			v |= u128(pop_fifo())<<96;
			return v;
		}
		
		u64 popd()
		{
			u64 v = pop_fifo();
			v |= u64(pop_fifo())<<32;
			return v;
		}
	} eedma;
	void ee_dma_chain(u32 chan, std::function<void(u32)> where);
	void ee_sif_dest_chain();

	struct {
		u32 SIF_MSCOM=0;
		u32 SIF_SMCOM=0;
		u32 SIF_MSFLG=0;
		u32 SIF_SMFLG=0;	
	} sifmb;
	
	struct {
		union {
			struct {
				unsigned int ctrl : 7;
				unsigned int pad0 : 8;
				unsigned int buserr : 1;
				unsigned int mask : 7;
				unsigned int ie : 1;
				unsigned int flag : 7;
				unsigned int mflag : 1;
			} PACKED b;
			u32 v;
		} PACKED DICR;

		union {
			struct {
				unsigned int tag : 13;
				unsigned int pad0 : 3;
				unsigned int mask : 6;
				unsigned int pad1 : 2;
				unsigned int flag : 6;
				unsigned int pad2 : 2;
			} PACKED b;
			u32 v;		
		} PACKED DICR2;

		u32 DPCR=0, DPCR2=0;
		u32 DMACEN=0, DMACINTEN=0;
		
		std::deque<u32> sif_fifo;
		u32 last_fifo=0;
		u32 pop_fifo() { if( !sif_fifo.empty() ) { last_fifo = sif_fifo.back(); sif_fifo.pop_back(); } return last_fifo; }
		
		u32 chan[13][4] = {0};
	} iop_dma;
	void iop_dma_ctrl(u32 c,u32 v);
	void iop_sif_dest_chain(); // called by ee_dma_chain
	
	struct {
		u32 I_CTRL=0;
		u32 I_MASK=0;
		u32 I_STAT=0;
	} iop_int;
	
	struct {
		u32 MCH_RICM=0;
		u32 MCH_DRD=0;
		u32 rdram_sdevid=0;
	} meminit;
};



