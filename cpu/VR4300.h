#pragma once
#include <functional>
#include "itypes.h"

struct BusResult
{
	BusResult() : result(5), value(0xbadf00d) {}
	BusResult(u64 v) : result(0), value(v) {}
	BusResult(u64 r, u64 v) : result(r), value(v) {}
	static BusResult exception(u64 res) { return BusResult{res,0}; }
	u64 result;
	u64 value;
	
	bool operator !() { return result!=0; }	
	operator u64() { return value; }
};

class VR4300
{
public:

	void step();
	void reset();
	void branch(u64);
	void overflow();
	void address_error(bool);
	void exception(u32 ec, u32 vector = 0x80000180);
	
	void invalidate(u32);
	
	u32 c0_read32(u32 reg);
	u64 c0_read64(u32 reg);
	void c0_write32(u32 reg, u64 v);
	void c0_write64(u32 reg, u64 v);
	
	bool COPUnusable(u32 cop);
	
	BusResult read(u64, int);
	BusResult write(u64, u64, int);
	
	std::function<u64(u32, int)> readers[((0x1FFFFFFF)+1)>>12];
	
	std::function<u64(u32, int)> phys_read;
	std::function<void(u32,u64,int)> phys_write;
	
	u64 pc, npc, nnpc;
	bool delay, ndelay;
	
	u64& INDEX = c[0];
	u64& RANDOM = c[1];
	u64& ENTRY_LO0 = c[2];
	u64& ENTRY_LO1 = c[3];
	u64& CONTEXT = c[4];
	u64& PAGE_MASK = c[5];
	u64& WIRED = c[6];
	u64& BADVADDR = c[8];
	u64& COUNT = c[9];
	u64& ENTRY_HI = c[10];
	u64& COMPARE = c[11];
	u64& STATUS = c[12];
	u64& CAUSE  = c[13];
	u64& EPC = c[14];
	u64& CONFIG = c[16];
	u64& LL_ADDR = c[17];
	u64& XCONTEXT = c[20];
	u64& ErrorEPC = c[30];
	
	u32 FCSR;
		
	u64 r[32];
	u64 c[32];
	u8 f[32*8];
	u64 hi, lo;
	bool LLbit, cop1_half_mode;
	
	const u64 STATUS_EXL = 2;
	const u64 STATUS_ERL = 4;
	
	bool signal_fpu(int sig)
	{
		return false;
		if( sig == FPU_UNIMPL )
		{
			FCSR |= BIT(17);
			exception(15);
			return true;
		}
		
		FCSR |= sig<<12;
		if( (FCSR>>7) & sig & 0x1F )
		{
			exception(15);
			return true;
		}
		
		FCSR |= sig<<2;
		return false;
	}
	
	const int FPU_INEXACT = 1;
	const int FPU_UNDERFLOW = 2;
	const int FPU_OVERFLOW = 4;
	const int FPU_DIVZERO = 8;
	const int FPU_INVALID = 0x10;
	const int FPU_UNIMPL = 0x20;
	
	bool fpu_cond;
	
	bool isQNaN_f(float);
	bool isQNaN_d(double);
	
	struct tlb_entry {
		u64 mask, page0, page1, vpn;
		u8 asid, G, C0, C1, V0, V1, D0, D1;
	} tlb[32];
	int tlb_search(u64 virt, u32& phys); // returns tlb entry # or -1
	
	bool in_infinite_loop;
	
	u8* RAM;
};

