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
	
	u32 c0_read32(u32 reg);
	u64 c0_read64(u32 reg);
	void c0_write32(u32 reg, u64 v);
	void c0_write64(u32 reg, u64 v);
	
	BusResult read(u64, int);
	BusResult write(u64, u64, int);
	
	std::function<u64(u32, int)> phys_read;
	std::function<void(u32,u64,int)> phys_write;
	
	u64 pc, npc, nnpc;
	bool delay, ndelay;
	
	u64& RANDOM = c[1];
	u64& CONTEXT = c[4];
	u64& WIRED = c[6];
	u64& BADVADDR = c[8];
	u64& COUNT = c[9];
	u64& COMPARE = c[11];
	u64& STATUS = c[12];
	u64& CAUSE  = c[13];
	u64& EPC = c[14];
	u64& LL_ADDR = c[17];
	
	u64& ErrorEPC = c[30];
		
	u64 r[32];
	u64 c[32];
	u8 f[32*8];
	u64 hi, lo;
	bool LLbit, cop1_half_mode;
	
	const u64 STATUS_EXL = 2;
	const u64 STATUS_ERL = 4;
};

