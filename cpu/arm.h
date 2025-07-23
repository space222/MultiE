#pragma once
#include <print>
#include <functional>
#include "itypes.h"

enum class ARM_CYCLE {  X, I, N, S };

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
	arm() 
	{ 
		cpsr.v = ARM_MODE_USER; 
		for(u32 i = 0; i < 16; ++i) 
		{ 
			r[i] = fiq[i] = 0; 
		}
		irq_line = false;
	}
	arm7flags cpsr;	
	u32 r[16];
	u32 fiq[16]; // only (8, 14) switch, but matching r for ease of use
	u32 r13_irq, r14_irq;
	u32 r13_abt, r14_abt;
	u32 r13_und, r14_und;
	u32 r13_svc, r14_svc;
	u32 spsr_svc, spsr_irq, spsr_fiq, spsr_abt, spsr_und;
	bool irq_line;
	
	void setUserReg(u32 reg, u32 val)
	{
		if( reg < 8 || reg == 15 ) { r[reg] = val; return; }
		if( cpsr.b.M == ARM_MODE_USER || cpsr.b.M == ARM_MODE_SYSTEM ) { r[reg] = val; return; }
		if( cpsr.b.M == ARM_MODE_FIQ ) { fiq[reg] = val; return; }
		if( reg < 13 ) { r[reg] = val; return; }
		switch( cpsr.b.M )
		{
		case ARM_MODE_IRQ: if( reg == 14 ) r14_irq = val; else r13_irq = val; return;
		case ARM_MODE_SUPER: if( reg == 14 ) r14_svc = val; else r13_svc = val; return;
		case ARM_MODE_UNDEF: if( reg == 14 ) r14_und = val; else r13_und = val; return;
		case ARM_MODE_ABORT: if( reg == 14 ) r14_abt = val; else r13_abt = val; return;
		default: break;
		}
		std::println("arm::setUserReg error mode ${:X} r{}", u32(cpsr.b.M), reg);
		exit(1);
	}
	
	u32 getUserReg(u32 reg)
	{
		if( reg < 8 || reg == 15 ) { return r[reg]; }
		if( cpsr.b.M == ARM_MODE_USER || cpsr.b.M == ARM_MODE_SYSTEM ) { return r[reg]; }
		if( cpsr.b.M == ARM_MODE_FIQ ) { return fiq[reg]; }
		if( reg < 13 ) { return r[reg]; }
		switch( cpsr.b.M )
		{
		case ARM_MODE_IRQ: if( reg == 14 ) return r14_irq; return r13_irq;
		case ARM_MODE_SUPER: if( reg == 14 ) return r14_svc; return r13_svc;
		case ARM_MODE_UNDEF: if( reg == 14 ) return r14_und; return r13_und;
		case ARM_MODE_ABORT: if( reg == 14 ) return r14_abt; return r13_abt;
		default: break;
		}
		std::println("arm::setUserReg error mode ${:X} r{}", u32(cpsr.b.M), reg);
		exit(1);
		return 0;
	}
	
	u32 RbyR; // dataproc adds this, set by the shifter if PC gets used in r shift by r situation
	ARM_CYCLE next_cycle_type;	
	//bool flushed;
	
	std::function<void(u32, u32, int, ARM_CYCLE)> write;
	std::function<u32(u32, int, ARM_CYCLE)> read;
	
	virtual void switch_to_mode(u32) =0;
	virtual void swi() =0;
	virtual bool isCond(u8) =0;
	virtual void flushp() =0;
	u64 stamp;
	
	u32 getSPSR() 
	{
		switch( cpsr.b.M )
		{
		case ARM_MODE_SUPER: return spsr_svc;
		case ARM_MODE_IRQ: return spsr_irq;
		case ARM_MODE_FIQ: return spsr_fiq;
		case ARM_MODE_ABORT: return spsr_abt;
		case ARM_MODE_UNDEF: return spsr_und;
		default: break;
		}
		return cpsr.v;
	}

	void setSPSR(u32 v)
	{
		switch( cpsr.b.M )
		{
		case ARM_MODE_SUPER: spsr_svc = v; return;
		case ARM_MODE_IRQ: spsr_irq = v; return;
		case ARM_MODE_FIQ: spsr_fiq = v; return;
		case ARM_MODE_ABORT: spsr_abt = v; return;
		case ARM_MODE_UNDEF: spsr_und = v; return;
		default: break;
		}
		std::println("${:X}: write to spsr in user||system mode", r[15]-4);
		return;		
	}
	
	void dump_regs();
	
	virtual ~arm() {}
};




