#pragma once
#include "itypes.h"

typedef u16 (*cp1600_reader)(u16);
typedef void (*cp1600_writer)(u16,u16);

class cp1600
{
public:
	u32 step();
	void reset();
	void irq() { irq_asserted = true; }
	void busrq_start() { busrq = true; }
	void busrq_end() { busrq = busak = false; }
	
	cp1600_reader read;
	cp1600_writer write;
	
	u16 mem(u16);
	void mem(u16, u16);
	
	u16 r[8];
	u8 f;
	bool D, I, interruptible;
	bool irq_asserted, busrq, busak;
	//bool halted;
	//int halt_cnt;
	
	u32 icyc;
	u64 stamp;
	
	void internal_ctrl(u16);
	void shift(u16);
	void branch(u16);
	void mvo(u16);
	void add(u16);
	void sub(u16);
	void cmp(u16);
	void op_and(u16);
	void op_xor(u16);
	void addr(u16);
	void subr(u16);
	void cmpr(u16);
	void andr(u16);
	void xorr(u16);
	void movr(u16);
	void incr(u16);
	void decr(u16);
	void adcr(u16);
	void comr(u16);
	void negr(u16);
	void gswd(u16);
	void rswd(u16);
	void jmp();
};

