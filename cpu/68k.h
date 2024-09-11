#pragma once
#include "itypes.h"

union flags
{
	struct {
		unsigned C : 1;
		unsigned V : 1;
		unsigned Z : 1;
		unsigned N : 1;
		unsigned X : 1;
		unsigned pad : 3;
		unsigned IPL : 3;
		unsigned pad2 : 2;
		unsigned S : 1;
		unsigned pad3 : 1;
		unsigned T : 1;
	} __attribute__((packed)) b;
	u16 raw;
} __attribute__((packed));

union mreg
{
	u8 B;
	u16 W;
	u32 L;
} __attribute__((packed));

typedef void (*m68k_write8)(u32, u8);
typedef void (*m68k_write16)(u32, u16);
typedef void (*m68k_write32)(u32, u32);
typedef u8 (*m68k_read8)(u32);
typedef u16 (*m68k_read16)(u32);
typedef u32 (*m68k_read32)(u32);
typedef void (*m68k_intack)();

struct m68k
{
	union {
		u32 r[17];
		mreg regs[17];
	};
	flags sr;
	u32 pc;
	
	void step();
	static void init();
	
	//bool hip, vip;
	u32 pending_irq;
	m68k_intack intack;
	
	m68k_write8 mem_write8;
	m68k_write16 mem_write16;
	m68k_write32 mem_write32;
	m68k_read8 mem_read8;
	m68k_read16 mem_read16;
	m68k_read16 read_code16;
	m68k_read32 mem_read32;
	
	u32 icycles;
	u64 stamp;
	bool halted;
	
	u32 autovector; // added for Jaguar. uses level 2 IRQ, but not addr 0x68
};


