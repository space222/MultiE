#pragma once
#include "itypes.h"

class arm;
enum class ARM_CYCLE {  X, I, N, S };

typedef void (*armwrite)(u32, u32, int, ARM_CYCLE);
typedef u32 (*armread)(u32, int, ARM_CYCLE);
typedef void (*arm7_instr)(arm&, u32);

#define ARM_MODE_USER 0x10
#define ARM_MODE_FIQ  0x11
#define ARM_MODE_IRQ  0x12
#define ARM_MODE_SUPER 0x13
#define ARM_MODE_ABORT 0x17
#define ARM_MODE_UNDEF 0x1B
#define ARM_MODE_SYSTEM 0x1F

union arm7flags
{
	struct {
		unsigned int M : 5;
		unsigned int T : 1;
		unsigned int F : 1;
		unsigned int I : 1;
		unsigned int resv : 20;
		unsigned int V : 1;
		unsigned int C : 1;
		unsigned int Z : 1;
		unsigned int N : 1;	
	} PACKED b;
	u32 v;
} PACKED;

class arm
{
public:

	arm7flags cpsr;	
	u32 r[16];
	u32 user[16];
	u32 fiq[16];
	u32 r13_irq, r14_irq;
	u32 r13_abt, r14_abt;
	u32 r13_und, r14_und;
	u32 r13_svc, r14_svc;
	u32 spsr_svc, spsr_irq, spsr_fiq, spsr_abt;
	
	ARM_CYCLE next_cycle_type;	
	bool flushed;
	
	armwrite write;
	armread read;
	
	virtual void switch_to_mode(u32);
	virtual void swi();
	
	virtual bool isCond(u8)=0;
	virtual void flushp() =0;
	u64 icycles;
	
	u32 getSPSR();
	void setCPSR(u32);
	void setSPSR(u32);
	
	void dump_regs();
};




