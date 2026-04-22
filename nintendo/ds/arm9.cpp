#include <print>
#include <string>
#include "nds.h"
#include "util.h"

u16 toggle = 0;

u32 nds::arm9_io_read(u32 a, int sz)
{
	if( a == 0x04000004u ) return toggle;
	if( a == 0x04000130u ) return (sz == 8) ? keystate&0xff : keystate;// keys();
	if( a == 0x04000180 )
	{ // IPCSYNC
		return (ipc.to_arm9&15)|((ipc.to_arm7&15)<<8)|(ipc.ipcsync9);
	}
	if( a == 0x04000208 ) return irq9.IME;
	if( a == 0x04000210 ) return irq9.IE;
	if( a == 0x04000214 ) return irq9.IF;
	std::println("IO Rd{} ${:X}", sz, a);
	return 0;
}

void nds::arm9_io_write(u32 a, u32 v, int sz)
{
	if( a == 0x04000180 )
	{ // IPCSYNC
		ipc.to_arm7 = (v>>8)&15;
		ipc.ipcsync9 = v & BIT(14);
		if( (v&BIT(13)) && (ipc.ipcsync7 & BIT(14)) )
		{
			arm7_raise_irq(IRQ_IPCSYNC_BIT);
		}
		return;
	}

	if( a == 0x04000208 ) { irq9.IME = v&1; return; }
	if( a == 0x04000210 ) { irq9.IE = v&0xffFFffu; arm9.irq_line = irq9.IME && (irq9.IE & irq9.IF); return; }
	if( a == 0x04000214 ) { irq9.IF &= ~v; arm9.irq_line = irq9.IME && (irq9.IE & irq9.IF); return; }

	std::println("IO Wr{} ${:X} = ${:X}", sz, a, v);
}

extern bool enditall;

u32 nds::arm9_read(u32 a, int sz, ARM_CYCLE)
{
	//todo: actual dtcm
	if( a >= arm9.dtcm.base && a < arm9.dtcm.base+arm9.dtcm.size ) return sized_read(dtcm, a&0x3fff, sz);

	if( a >= 0x04000000u && a < 0x05000000u )
	{
		return arm9_io_read(a, sz);
	}
	if( a >= 0x02000000u && a < 0x03000000u )
	{
		return sized_read(mainram, a&(4_MB-1), sz);
	}
	if( a >= 0x06800000u && a < 0x07000000u )
	{
		return sized_read(vram, a-0x06800000u, sz);
	}
	if( a >= 0xffff0000u ) { return sized_read(arm9_bios, a&0x7fff, sz); }
	std::println("arm9 rd{} ${:X}", sz, a);
	return 0;
}

void nds::arm9_write(u32 a, u32 v, int sz, ARM_CYCLE)
{
	if( a >= arm9.dtcm.base && a < arm9.dtcm.base+arm9.dtcm.size ) return sized_write(dtcm, a&0x3fff, v, sz);
	if( a >= 0x02000000u && a < 0x03000000u )
	{
		return sized_write(mainram, a&(4_MB-1), v, sz);
	}
	if( a >= 0x04000000u && a < 0x05000000u )
	{
		return arm9_io_write(a, v, sz);
	}
	if( a >= 0x06800000u && a < 0x07000000u )
	{
		//std::println("vram ${:X} = ${:X}", a,v);
		return sized_write(vram, a-0x06800000u, v, sz);
	}

	if( a == 0x04000180 ) return;
	std::println("arm9 wr{} ${:X} = ${:X}", sz, a, v);
}

u32 nds::arm9_fetch(u32 a, int sz, ARM_CYCLE)
{
	if( a >= 0x02000000u && a < 0x03000000u )
	{
		return sized_read(mainram, a&(4_MB-1), sz);
	}
	if( a >= 0xFFFF0000u ) return sized_read(arm9_bios, a&0x3fff, sz);
	std::println("arm9 fetch{} ${:X}", sz, a);
	return 0;
}

void nds::arm9_raise_irq(u32 bit)
{
	irq9.IF |= bit;
	arm9.irq_line = irq9.IME && (irq9.IF & irq9.IE);
	if( arm9.irq_line ) { irq9.halted = 0; }
}

void nds::arm9_clear_irq(u32 bit)
{
	irq9.IF &= ~bit;
	arm9.irq_line = irq9.IME && (irq9.IF & irq9.IE);
}


