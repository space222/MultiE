#pragma once
#include <print>
#include <deque>
#include "Scheduler.h"
#include "console.h"
#include "EEngine.h"
#include "r3000.h"

extern bool logall;

class ps2 : public console
{
public:
	ps2() : sched(this) {}
	u32 fb_width() override { return 512; }
	u32 fb_height() override { return 256; }
	u32 fb_bpp() override { return 34; }
	
	u8* framebuffer() override 
	{ 
		u64 frame = gs.regs[0x4C + ((gs.regs[0]&BIT(9))?1:0)];
		return (u8*)&vram[(frame & 0x1ff)*2048*4]; 
	}
	
	void run_frame() override;
	bool loadROM(std::string) override;
	void reset() override;
	
	void event(u64, u32) override;
		
	bool loaded_elf = false;
	u32 elf_entry = 0xbad0e1f;
	
	void key_down(int f)
	{ 
		if( f == SDL_SCANCODE_ESCAPE ) logall = !logall;
		if( f == SDL_SCANCODE_F ) { std::println("IOP Stat = ${:X}", iop.c[12]); }
	}
	
	//bool logall = false;
	
	void iop_write(u32 a, u32 v, int sz);
	u32 iop_read(u32 a, int sz);
	
	u128 read(u32, int);
	void write(u32, u128, int);
	
	u64 global_stamp=0, iop_stamp=0;
	u64 last_target=0;

	Scheduler sched;
	EEngine cpu;
	r3000 iop;

	u8 BIOS[4_MB];
	u8 spad[16_KB];
	u8 RAM[32_MB];
	u8 vram[4_MB];
	
	u8 iop_spad[1_KB];
	u8 iop_ram[2_MB];
	
	u32 fbuf[640*480];
	
	const u32 EVENT_VBLANK = 1;
	
	const u32 CSR_VBINT = BIT(3);
	const u32 CSR_FIELD = BIT(13);
	struct {
		u32 PMODE=0, SMODE2=0, DISPFB1=0, DISPLAY1=0, DISPFB2=0, DISPLAY2=0;
		u32 EXTBUF=0, EXTDATA=0, EXTWRITE=0, BGCOLOR=0, CSR=0;
		u32 IMR=0, BUSDIR=0, SIGLBLID=0;
		
		u32 dx=0, dy=0;
		
		std::deque<u128> fifo;
		u64 regs[256];
	} gs;
	struct vertex
	{
		u32 color;
		float z;
		float x, y;
		float s, t, q;
		u8 fog;
	};

	void gs_run_fifo();	
	void gs_vertex_out(u64 vert, bool kick);
	void gs_draw_line(vertex& a, vertex& b);
	void gs_draw_sprite(vertex& a, vertex& b);
	void gs_draw_triangle(vertex& a, vertex& b, vertex& c);
	void gs_set_pixel(u32 x, u32 y, u32 c);
	void gs_hwreg_poke(u64);
	
	struct {
		std::deque<u8> Sparam;
		std::deque<u8> Sres;
		
	
	} cdvd;
	
	struct {
		u32 INTC_STAT=0;
		u32 INTC_MASK=0;
	} eeint;
	
	struct {
		bool sif_active = false;
		u32 D_STAT=0, D_CTRL=0, D_PCR=0, D_SQWC=0, D_STADR=0;
		u32 D_RBSR=0, D_RBOR=0, D_ENABLE=0x1201;
		
		struct {
			u32 chcr, madr, tadr, qwc, asr0, asr1, sadr;		
		} chan[10] = {};
		
		
		std::deque<u128> sif_fifo;
		u32 last_fifo=0;
		u32 pop_fifo() { if( !sif_fifo.empty() ) { last_fifo = sif_fifo.back(); sif_fifo.pop_back(); } return last_fifo; }

		u128 popq()
		{
			u128 v = popd();
			v |= u128(popd())<<64;
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
		bool sif_active = false;
		
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



