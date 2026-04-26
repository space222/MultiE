#include <print>
#include <string>
#include "util.h"
#include "nds.h"

u32 nds::arm7_io_read(u32 a, int sz)
{
	//std::println("arm7 io rd{} ${:X}", sz, a);
	if( a == 0x040001A4 )
	{
		return BIT(23);
	}
	if( a == 0x04000180 )
	{ // IPCSYNC
		return (ipc.to_arm7&15)|((ipc.to_arm9&15)<<8)|(ipc.ipcsync7);
	}
	if( a == 0x04000184 ) return ipc.fifocnt7.v;
	if( a == 0x04000208 ) return irq7.IME;
	if( a == 0x04000210 ) return irq7.IE;
	if( a == 0x04000214 ) return irq7.IF;
	if( a == 0x04000241 ) return wramcnt;
	if( a == 0x04100000 )
	{ // IPC Receive FIFO
		auto& othercnt = ipc.fifocnt9;
		auto& thiscnt = ipc.fifocnt7;
		auto& Q = ipc.q2arm7;
		if( thiscnt.b.enable == 0 ) return (Q.empty() ? ipc.last_q2arm7 : Q.front());
		if( thiscnt.b.recv_empty )
		{
			thiscnt.b.error = 1;
			return ipc.last_q2arm7;
		}
		
		u32 retval = ipc.last_q2arm7 = Q.front(); Q.pop_front();
		othercnt.b.send_full = thiscnt.b.recv_full = 0;
		if( Q.empty() )
		{
			u32 oldirq = othercnt.b.send_empty & othercnt.b.send_empty_irq_en;
			othercnt.b.send_empty = 1;
			if( !oldirq && (othercnt.b.send_empty & othercnt.b.send_empty_irq_en) )
			{
				arm9_raise_irq(IRQ_IPC_SEND_EMPTY);
			}
			thiscnt.b.recv_empty = 1;
		}
		return retval;
	}
	
	std::println("arm7 io rd{} ${:X}", sz, a);
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
	
	if( a == 0x04000184 )
	{ // IPCFIFOCNT
		u32 send_empty_old = ipc.fifocnt7.b.send_empty_irq_en & ipc.fifocnt7.b.send_empty;
		u32 recv_nempty_old = ipc.fifocnt7.b.recv_nempty_irq_en & !ipc.fifocnt7.b.recv_empty;
		
		fifocnt_t f;
		f.v = v;
		if( f.b.error ) ipc.fifocnt7.b.error = 0;
		ipc.fifocnt7.b.send_empty_irq_en = f.b.send_empty_irq_en;
		ipc.fifocnt7.b.recv_nempty_irq_en = f.b.recv_nempty_irq_en;
		ipc.fifocnt7.b.enable = f.b.enable;
				
		if( send_empty_old == 0 && (ipc.fifocnt7.b.send_empty_irq_en & ipc.fifocnt7.b.send_empty) )
		{
			arm7_raise_irq(IRQ_IPC_SEND_EMPTY);
		}
		if( recv_nempty_old == 0 && (ipc.fifocnt7.b.recv_nempty_irq_en & !ipc.fifocnt7.b.recv_empty) )
		{
			arm7_raise_irq(IRQ_IPC_RECV_NEMPTY);
		}
		
		if( f.b.send_clear )
		{
			ipc.q2arm9.clear();
			ipc.last_q2arm9 = 0;
			ipc.fifocnt7.b.send_empty = ipc.fifocnt9.b.recv_empty = 1;
			ipc.fifocnt7.b.send_full = ipc.fifocnt9.b.recv_full = 0;
		}
		return;
	}
	
	if( a == 0x04000188 )
	{ // IPCFIFOSEND
		auto& othercnt = ipc.fifocnt9;
		auto& thiscnt = ipc.fifocnt7;
		auto& Q = ipc.q2arm9;
		if( thiscnt.b.enable == 0 ) return;
		if( thiscnt.b.send_full )
		{
			thiscnt.b.error = 1;
			return;
		}
		if( Q.empty() )
		{
			if( othercnt.b.recv_empty && othercnt.b.recv_nempty_irq_en )
			{
				arm9_raise_irq(IRQ_IPC_RECV_NEMPTY);
			}
			othercnt.b.recv_empty = thiscnt.b.send_empty = 0;
		}
		Q.push_back(v);
		if( Q.size() >= 16 )
		{
			thiscnt.b.send_full = othercnt.b.recv_full = 1;
		}
		return;
	}
	
	if( a == 0x04000208 ) { irq7.IME = v&1; return; }
	if( a == 0x04000210 ) { irq7.IE = v&0xffFFffu; arm7.irq_line = irq7.IME && (irq7.IE & irq7.IF); return; }
	if( a == 0x04000214 ) { irq7.IF &= ~v; arm7.irq_line = irq7.IME && (irq7.IE & irq7.IF); return; }

	std::println("arm7 io wr{} ${:X} = ${:X}", sz, a, v);
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
	if( irq7.IF & irq7.IE ) { arm7.halted = 0; }
}

void nds::arm7_clear_irq(u32 bit)
{
	irq7.IF &= ~bit;
	arm7.irq_line = irq7.IME && (irq7.IF & irq7.IE);
}

