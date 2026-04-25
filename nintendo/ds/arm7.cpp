#include <print>
#include <string>
#include "util.h"
#include "nds.h"

u32 nds::arm7_io_read(u32 a, int sz)
{
	if( a == 0x04000180 )
	{ // IPCSYNC
		return (ipc.to_arm7&15)|((ipc.to_arm9&15)<<8)|(ipc.ipcsync7);
	}
	if( a == 0x04000208 ) return irq7.IME;
	if( a == 0x04000210 ) return irq7.IE;
	if( a == 0x04000214 ) return irq7.IF;
	if( a == 0x04000241 ) return wramcnt;
	if( a == 0x04100000 )
	{ // IPC Receive FIFO
		if( !(ipc.fifocnt7 & BIT(15)) )
		{
			if( ipc.q2arm7.empty() ) 
			{
				return ipc.last_q2arm7;
			}
			return ipc.q2arm7.front();
		}
		if( ipc.q2arm7.empty() )
		{
			ipc.fifocnt7 |= BIT(14);
			return ipc.last_q2arm7;
		}
		u32 val = ipc.q2arm7.front(); ipc.q2arm7.pop_front();
		if( ipc.q2arm7.empty() )
		{
			u32 oldstat = ipc.fifocnt9 & 1; // empty status
			ipc.fifocnt9 |= 1;
			if( oldstat==0 && (ipc.fifocnt9&5)==5 )
			{
				arm9_raise_irq(IRQ_IPC_SEND_EMPTY);
			}
			ipc.fifocnt7 |= BIT(8);	
		}
		return ipc.last_q2arm7 = val;
	}
	return 0;
}

void nds::arm7_io_write(u32 a, u32 v, int sz)
{
	if( a == 0x04000180 )
	{ // IPCSYNC
		ipc.to_arm9 = (v>>8)&15;
		ipc.ipcsync7 = v & BIT(14);
		if( (v&BIT(13)) && (ipc.ipcsync9 & BIT(14)) )
		{
			arm9_raise_irq(IRQ_IPCSYNC_BIT);
		}		
		return;
	}
	
	if( a == 0x04000208 ) { irq7.IME = v&1; return; }
	if( a == 0x04000210 ) { irq7.IE = v&0xffFFffu; arm7.irq_line = irq7.IME && (irq7.IE & irq7.IF); return; }
	if( a == 0x04000214 ) { irq7.IF &= ~v; arm7.irq_line = irq7.IME && (irq7.IE & irq7.IF); return; }

}

u32 nds::arm7_read(u32 a, int sz, ARM_CYCLE)
{
	if( a < 0x02000000 ) return sized_read(arm7_bios, a&0x3fff, sz);
	if( a < 0x03000000 ) return sized_read(mainram, a&(4_MB-1), sz);
	if( a >= 0x03000000u && a < 0x03800000u )
	{
		switch( wramcnt&3 )
		{
		case 3: a &= 0x7fff; break; 		
		case 2: a = 0x4000 + (a&0x3fff); break;
		case 1: a &= 0x3fff; break;
		case 0: return sized_read(arm7_wram, a&0xffff, sz);	
		}
		return sized_read(shared_wram, a, sz);	
	}
	if( a >= 0x03800000 && a < 0x04000000 ) return sized_read(arm7_wram, a&0xffff, sz);
	if( a >= 0x04000000 && a < 0x05000000 ) return arm7_io_read(a, sz);
	std::println("arm7 rd{} ${:X}", sz, a);
	return 0;
}

void nds::arm7_write(u32 a, u32 v, int sz, ARM_CYCLE)
{
	if( a < 0x02000000 ) return; // no write to bios
	if( a < 0x03000000 ) return sized_write(mainram, a&(4_MB-1), v, sz);
	if( a >= 0x03000000u && a < 0x03800000u )
	{
		switch( wramcnt&3 )
		{
		case 3: a &= 0x7fff; break; 		
		case 2: a = 0x4000 + (a&0x3fff); break;
		case 1: a &= 0x3fff; break;
		case 0: return sized_write(arm7_wram, a&0xffff, v, sz);	
		}
		return sized_write(shared_wram, a, v, sz);	
	}
	if( a >= 0x03800000 && a < 0x04000000 ) return sized_write(arm7_wram, a&0xffff, v, sz);
	if( a >= 0x04000000 && a < 0x05000000 ) return arm7_io_write(a, v, sz);
	std::println("${:X}: arm7 wr{} ${:X} = ${:X}", arm7.r[15] - (arm7.cpsr.b.T ? 4:8), sz, a, v);
}


void nds::arm7_raise_irq(u32 bit)
{
	irq7.IF |= bit;
	arm7.irq_line = irq7.IME && (irq7.IF & irq7.IE);
	if( arm7.irq_line ) { arm7.halted = 0; }
}

void nds::arm7_clear_irq(u32 bit)
{
	irq7.IF &= ~bit;
	arm7.irq_line = irq7.IME && (irq7.IF & irq7.IE);
}

