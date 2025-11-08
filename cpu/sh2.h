#pragma once
#include <functional>
#include "itypes.h"

class sh2
{
public:
	void init();
	
	u32 r[16];
	u8 irq_line;
	bool sleeping = false;
		
	std::function<u64(u32, u32)> bus_read;
	std::function<void(u32,u64,u32)> bus_write;
	std::function<u16(u32)> fetch = [&](u32 a)->u16 { return bus_read(a,16); };

	std::function<void(u32)> pref;
	
	u64 read(u32 a, u32 sz) { return bus_read(a,sz); }
	void write(u32 a, u64 v, u32 sz) { bus_write(a,v,sz); }	
		
	union sh2_sr_t
	{
		struct {
			unsigned int T : 1;
			unsigned int S : 1;
			unsigned int pad2 : 2;
			unsigned int IMASK : 4;
			unsigned int Q : 1;
			unsigned int M : 1;
			unsigned int pad22 : 22;
		} b PACKED;
		u32 v;
	} sr PACKED;
	
	u32 pc, PR, SSR, SPC, SGR, GBR, VBR, DBR, MACH, MACL;
	u32 TRA, EXPEVT, INTEVT;

	void step();
	void exec(u16);
	void setSR(u32);
	
	void div1(u32,u32);
	
	void reset() {}
	
	std::function<void(sh2&,u16)> opcache[0x10000];
};

