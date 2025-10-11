#pragma once
#include <functional>
#include "itypes.h"

class sh4
{
public:
	void init();
	
	u32 r[16];
	u32 rbank[16];
	u8 irq_line;
	bool sleeping = false;
	
	union {
		float f[32];
		double d[16];
	} fpu;
	
	std::function<u64(u32, u32)> bus_read;
	std::function<void(u32,u64,u32)> bus_write;
	
	std::function<u16(u32)> fetch = [&](u32 a)->u16 { return bus_read(a,16); };
	
	u64 read(u32 a, u32 sz) { return bus_read(a,sz); }
	void write(u32 a, u64 v, u32 sz) { bus_write(a,v,sz); }	
		
	union sh4_sr_t
	{
		struct {
			unsigned int T : 1;
			unsigned int S : 1;
			unsigned int pad2 : 2;
			unsigned int IMASK : 4;
			unsigned int Q : 1;
			unsigned int M : 1;
			unsigned int pad5 : 5;
			unsigned int FD : 1;
			unsigned int pad12 : 12;
			unsigned int BL : 1;
			unsigned int RB : 1;
			unsigned int MD : 1;
			unsigned int pad1 : 1;
		} b PACKED;
		u32 v;
	} sr PACKED;
	
	union {
		struct {
			unsigned int RM : 2;
			unsigned int Flag : 5;
			unsigned int Enable : 5;
			unsigned int Cause : 6;
			unsigned int DN : 1;
			unsigned int PR : 1;
			unsigned int SZ : 1;
			unsigned int FR : 1;
			unsigned int pad : 10;
		} b PACKED;
		u32 v;
	} fpctrl PACKED;

	u32 pc, PR, SSR, SPC, SGR, GBR, VBR, DBR, MACH, MACL, FPUL;
	u32 TRA, EXPEVT, INTEVT;

	void step();
	void exec(u16);
	void setSR(u32);
	
	void div1(u32,u32);
	void fipr(u32,u32);
	void ftrv(u32);
	void fpu_disabled() {}
	void priv();
	void swapregs();
	
	std::function<void(sh4&,u16)> opcache[0x10000];
};

