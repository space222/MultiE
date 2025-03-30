#pragma once
#include <cstdint>

typedef uint32_t u32;
typedef uint64_t u64;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

struct arm7
{
	u32 regs[16];
	u32 fiq_regs[16];
	u32 cpsr;
	u32 spsr_fiq, spsr_svc, spsr_abt, spsr_irq, spsr_und;
	u32 r14_svc, r14_irq, r14_abt;
	u32 r13_svc, r13_irq, r13_abt;
	u32 FLAG_Z, FLAG_C, FLAG_N, FLAG_V;
	u32 prefetch;
	bool preset;
	u32 Icycles, Ncycles, Scycles;
	u32 Istates, Nstates, Sstates;
};

#define SBIT(a) ((a)&(1ul<<20))

#define MODE_USER 0x10
#define MODE_FIQ  0x11
#define MODE_IRQ  0x12
#define MODE_SUPER 0x13
#define MODE_ABORT 0x17
#define MODE_UNDEF 0x1B
#define MODE_SYSTEM  0x1F

#define CPSR_IRQ 0x80

#define DEBUG_STEP 0
#define DEBUG_UNTIL 1
#define DEBUG_BREAKPOINT 2











