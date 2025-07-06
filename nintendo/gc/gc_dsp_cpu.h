#pragma once
#include "itypes.h"

class GCDsp
{
public:
	u16 r[32];
	u16 pc;

	struct {
		u16 csr;
		
		u32 ramaddr, aramaddr, aramdmactrl;
		u16 dspboxH, dspboxL;
		u16 cpuboxH, cpuboxL;
		u16 AR_SIZE, AR_REFRESH; // not useful for emu, but are r/w
	} io;

	void step();
	u16 fetch(u16);
	u16 read(u16);
	void write(u16, u16);
	
	u16 readio(u16);
	void writeio(u16, u16);
	
	u8 irom[8*1024*1024];	// loaded from host disk
	u8 dram[8*1024*1024];	// dsp's work ram

	u8 aram[16*1024*1024];  // iram is low 8k of this
	u8 coef[4*1024*1024];   // drom but with less similar name as the dram var
};

