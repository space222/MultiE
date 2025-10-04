#pragma once
#include <functional>
#include "itypes.h"



class z80n
{
public:
	u8 r[8];
	u8 rbank[8];
	u8 I, R;
	u16 ix, iy;
	u16 pc, sp;
	s8 disp8;
	u8 prefix;
	
	u8 table_r(u32 reg, bool use_override = true);	
	void table_r(u32 reg, u8 v, bool use_override = true);
	
	u16 rp(u32);
	u16 rp2(u32);
	void rp(u32, u16);
	void rp2(u32, u16);
	
	bool cond(u8);
	u8 setszp(const u8);
	void set53(u8);
	void setH(u8);
	void setV(u8);
	void setC(u8);
	void setN(u8);
	void setZ(u8);

	std::function<u8(u16)> read8;
	std::function<void(u16, u8)> write8;
	
	std::function<void(u16, u8)> out8;
	std::function<u8(u16)> in8;

	u64 step() { step_exec(); return 4; }
	void step_exec();
	void reset();
	
	void exec_ed(u8);
	void exec_cb(u8);
	
	void alu(const u32 op, u8);
	void block(u8,u8);
	
	void push16(u16);
	u16 pop16();
	void rst(u8);
	u8 inc8(u8);
	u8 dec8(u8);
	void daa();
	
	u8 imm8();
	u16 imm16();
	
	void setHL(u16);
	
	void load_disp()
	{
		disp8 = read8(pc);
		pc+=1;
	}
	
	u16 read16(u16 a)
	{
		u16 res = read8(a);
		res |= read8(a+1)<<8;
		return res;	
	}
	
	void write16(u16 a, u16 v)
	{
		write8(a, v);
		write8(a+1, v>>8);	
	}
		
	bool irq_blocked, iff1, iff2, halted;
	u8 irq_line;
	
	u64 stamp;
};

