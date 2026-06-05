#pragma once
#include "itypes.h"

class GCDsp
{
public:
	u16 r[32];
	u16 pc;

	union flag_t
	{
		struct {
			bitfield c : 1;
			bitfield o : 1;
			bitfield z : 1;
			bitfield s : 1;
			bitfield as : 1;
			bitfield tb : 1;
			bitfield lz : 1;
			bitfield os: 1;
			bitfield pad0 : 1;
			bitfield ie : 1;
			bitfield pad1 : 1;
			bitfield eie : 1;
			bitfield pad2 : 1;
			bitfield am : 1;
			bitfield sxm : 1;
			bitfield su : 1;		
		} PACKED b;	
		u32 v;
	} PACKED F;

	struct {
		u16 csr;
		
		u32 ramaddr, aramaddr, aramdmactrl;
		u16 dspboxH, dspboxL;
		u16 cpuboxH, cpuboxL;
		u16 AR_SIZE, AR_REFRESH; // not useful for emu, but are r/w
	} io;
	
	s64 ac(u32 ind)
	{
		ind &= 1;
		s64 val = (u64(r[16+ind])<<32)|(r[30+ind]<<16)|r[28+ind];
		return (val<<24)>>24;
	}
	
	void step();
	void reset();
	u16 fetch(u16);
	u16 read(u16);
	void write(u16, u16);
	
	u16 readio(u16);
	void writeio(u16, u16);
	
	u16 irom[4*1024];	// loaded from host disk
	u16 dram[4*1024];	// dsp's work ram

	u8 aram[16*1024*1024];  // iram is low 8k of this
	u16 coef[2*1024];   // drom but with less similar name as the dram var
};

