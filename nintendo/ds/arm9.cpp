#include <print>
#include <string>
#include "nds.h"
#include "util.h"


u32 nds::arm9_io_read(u32 a, int sz)
{
	if( a == 0x04000004u ) return disp.stat;
	if( a == 0x04000006u ) return disp.scanline;
	if( a == 0x04000130u ) return (sz == 8) ? keystate&0xff : keystate;// keys();
	if( a == 0x04000180 )
	{ // IPCSYNC
		return (ipc.to_arm9&15)|((ipc.to_arm7&15)<<8)|(ipc.ipcsync9);
	}
	if( a == 0x04000184 ) return ipc.fifocnt9.v;
	if( a == 0x04000208 ) return irq9.IME;
	if( a == 0x04000210 ) return irq9.IE;
	if( a == 0x04000214 ) return irq9.IF;
	if( a == 0x04000247 ) return wramcnt;
	if( a >= 0x04000240 && a <= 0x04000249 ) { return sized_read(vmap_bytes, a-0x04000240, sz); }

	if( a == 0x04100000 )
	{ // IPC Receive FIFO
		auto& othercnt = ipc.fifocnt7;
		auto& thiscnt = ipc.fifocnt9;
		auto& Q = ipc.q2arm9;
		if( thiscnt.b.enable == 0 ) return (Q.empty() ? ipc.last_q2arm9 : Q.front());
		if( thiscnt.b.recv_empty )
		{
			thiscnt.b.error = 1;
			return ipc.last_q2arm9;
		}
		
		u32 retval = ipc.last_q2arm9 = Q.front(); Q.pop_front();
		othercnt.b.send_full = thiscnt.b.recv_full = 0;
		if( Q.empty() )
		{
			u32 oldirq = othercnt.b.send_empty & othercnt.b.send_empty_irq_en;
			othercnt.b.send_empty = 1;
			if( !oldirq && (othercnt.b.send_empty & othercnt.b.send_empty_irq_en) )
			{
				arm7_raise_irq(IRQ_IPC_SEND_EMPTY);
			}
			thiscnt.b.recv_empty = 1;
		}	
		return retval;
	}
	
	if( a == 0x04000280 ) return dsmath.divcnt;
	if( a == 0x04000290 ) return dsmath.div_num;
	if( a == 0x04000294 ) return dsmath.div_num>>32;
	if( a == 0x04000298 ) return dsmath.div_den;
	if( a == 0x0400029C ) return dsmath.div_den>>32;		
	if( a == 0x040002A0 ) return dsmath.div_quot;
	if( a == 0x040002A4 ) return dsmath.div_quot>>32;
	if( a == 0x040002A8 ) return dsmath.div_rem;
	if( a == 0x040002AC ) return dsmath.div_rem>>32;
	
	if( a == 0x040002B0 ) return dsmath.sqrtcnt;
	if( a == 0x040002B4 ) return dsmath.sqrt_res;
	if( a == 0x040002B8 ) return dsmath.sqrt_param;
	if( a == 0x040002BC ) return dsmath.sqrt_param>>32;
	std::println("arm9 IO Rd{} ${:X}", sz, a);
	return 0;
}

void nds::arm9_io_write(u32 a, u32 v, int sz)
{
	if( a == 0x04000208 ) { irq9.IME = v&1; return; }
	if( a == 0x04000210 ) { irq9.IE = v&0xffFFffu; arm9.irq_line = irq9.IME && (irq9.IE & irq9.IF); return; }
	if( a == 0x04000214 ) { irq9.IF &= ~v; arm9.irq_line = irq9.IME && (irq9.IE & irq9.IF); return; }

	if( a == 0x04000004 )
	{
		if( sz != 16 ) { std::println("dispstat wr{} isn't 16", sz); exit(1); }
		disp.stat &= 7;
		disp.stat |= (v&~7);
		return;
	}
	if( a == 0x04000180 )
	{ // IPCSYNC
		//std::println("arm9 ipcsync = ${:X}", v);
		ipc.to_arm7 = (v>>8)&15;
		ipc.ipcsync9 = v & BIT(14);
		if( (v&BIT(13)) && (ipc.ipcsync7 & BIT(14)) )
		{
			arm7_raise_irq(IRQ_IPCSYNC_BIT);
		}
		return;
	}
	
	if( a == 0x04000184 )
	{ // IPCFIFOCNT
		u32 send_empty_old = ipc.fifocnt9.b.send_empty_irq_en & ipc.fifocnt9.b.send_empty;
		u32 recv_nempty_old = ipc.fifocnt9.b.recv_nempty_irq_en & !ipc.fifocnt9.b.recv_empty;
		
		fifocnt_t f;
		f.v = v;
		if( f.b.error ) ipc.fifocnt9.b.error = 0;
		ipc.fifocnt9.b.send_empty_irq_en = f.b.send_empty_irq_en;
		ipc.fifocnt9.b.recv_nempty_irq_en = f.b.recv_nempty_irq_en;
		ipc.fifocnt9.b.enable = f.b.enable;
				
		if( send_empty_old == 0 && (ipc.fifocnt9.b.send_empty_irq_en & ipc.fifocnt9.b.send_empty) )
		{
			arm9_raise_irq(IRQ_IPC_SEND_EMPTY);
		}
		if( recv_nempty_old == 0 && (ipc.fifocnt9.b.recv_nempty_irq_en & !ipc.fifocnt9.b.recv_empty) )
		{
			arm9_raise_irq(IRQ_IPC_RECV_NEMPTY);
		}
		
		if( f.b.send_clear )
		{
			ipc.q2arm7.clear();
			ipc.last_q2arm7 = 0;
			ipc.fifocnt9.b.send_empty = ipc.fifocnt7.b.recv_empty = 1;
			ipc.fifocnt9.b.send_full = ipc.fifocnt7.b.recv_full = 0;
		}
		return;
	}
	if( a == 0x04000188 )
	{ // IPCFIFOSEND
		auto& othercnt = ipc.fifocnt7;
		auto& thiscnt = ipc.fifocnt9;
		auto& Q = ipc.q2arm7;
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
				arm7_raise_irq(IRQ_IPC_RECV_NEMPTY);
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

	
	if( a >= 0x04000240 && a <= 0x04000249 )
	{
		sized_write(vmap_bytes, a-0x04000240, v, sz);
		wramcnt = vmap_bytes[7]&3;
		remap_vram();
		return;
	}
	
	if( a == 0x04000280 )
	{
		dsmath.divcnt = v&3;
		dsmath_div();
		return;
	}
	if( a == 0x04000290 )
	{
		dsmath.div_num &= 0xFFFFffff00000000ULL;
		dsmath.div_num |= v;
		dsmath_div();
		return;
	}
	if( a == 0x04000294 )
	{
		dsmath.div_num &= 0xFFFFffffULL;
		dsmath.div_num |= u64(v)<<32;
		dsmath_div();
		return;
	}
	if( a == 0x04000298 )
	{
		dsmath.div_den &= 0xFFFFffff00000000ULL;
		dsmath.div_den |= v;
		dsmath_div();
		return;
	}
	if( a == 0x0400029C )
	{
		dsmath.div_den &= 0xFFFFffffULL;
		dsmath.div_den |= u64(v)<<32;
		dsmath_div();
		return;
	}
	if( a == 0x040002B0 )
	{
		dsmath.sqrtcnt = (v&1);
		dsmath_sqrt();
		return;
	}
	if( a == 0x040002B8 )
	{
		dsmath.sqrt_param &= 0xffffFFFF00000000ULL;
		dsmath.sqrt_param |= v;
		dsmath_sqrt();
		return;
	}
	if( a == 0x040002BC )
	{
		dsmath.sqrt_param &= 0xffffFFFFULL;
		dsmath.sqrt_param |= u64(v)<<32;
		dsmath_sqrt();
		return;
	}
std::println("arm9 IO Wr{} ${:X} = ${:X}", sz, a, v);	
}

extern bool enditall;

u32 nds::arm9_read(u32 a, int sz, ARM_CYCLE)
{
	//todo: actual dtcm
	if( a >= arm9.dtcm.base && a < arm9.dtcm.base+arm9.dtcm.size ) return sized_read(dtcm, a&0x3fff, sz);
	if( a >= 0x01000000u && a < 0x02000000u )
	{ //todo: real'er itcm
		return sized_read(itcm, a&(32_KB-1), sz);
	}
	if( a >= 0x03000000u && a < 0x04000000u )
	{
		switch( wramcnt&3 )
		{
		case 0: a &= 0x7fff; break; 		
		case 1: a = 0x4000 + (a&0x3fff); break;		
		case 2: a &= 0x3fff; break;		
		case 3: return 0;	
		}
		return sized_read(shared_wram, a, sz);	
	}
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
	if( a >= 0x01000000u && a < 0x02000000u )
	{ //todo: real'er itcm
		return sized_write(itcm, a&(32_KB-1), v, sz);
	}
	if( a >= 0x02000000u && a < 0x03000000u )
	{
		return sized_write(mainram, a&(4_MB-1), v, sz);
	}
	if( a >= 0x03000000u && a < 0x04000000u )
	{
		switch( wramcnt&3 )
		{
		case 0: a &= 0x7fff; break; 		
		case 1: a = 0x4000 + (a&0x3fff); break;		
		case 2: a &= 0x3fff; break;		
		case 3: return;	
		}
		return sized_write(shared_wram, a, v, sz);	
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

	std::println("arm9 wr{} ${:X} = ${:X} '{:c}'", sz, a, v, (char)v);
}

u32 nds::arm9_fetch(u32 a, int sz, ARM_CYCLE)
{
	if( a >= 0x01000000u && a < 0x02000000u )
	{
		return sized_read(itcm, a&(32_KB-1), sz);
	}
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
	if( arm9.irq_line ) { arm9.halted = 0; }
}

void nds::arm9_clear_irq(u32 bit)
{
	irq9.IF &= ~bit;
	arm9.irq_line = irq9.IME && (irq9.IF & irq9.IE);
}

void nds::remap_vram()
{
	for(u32 i = 0; i < 512/4; ++i) engineA_bg[i] = nullptr;
	for(u32 i = 0; i < 128/4; ++i) engineB_bg[i] = nullptr;
	for(u32 i = 0; i < 256/4; ++i) engineA_obj[i] = nullptr;
	for(u32 i = 0; i < 128/4; ++i) engineB_obj[i] = nullptr;
	
	if( vmap_bytes[0] & 0x80 )
	{
		u32 mst = vmap_bytes[0] & 7;
		u32 ofs = (vmap_bytes[0]>>3)&3;
		if( mst == 1 || mst == 2 )
		{
			if( mst == 2 ) ofs &= 1;
			u8** temp = 0x20*ofs + ((mst==1) ? engineA_bg : engineA_obj);
			for(u32 i = 0; i < 32; ++i) temp[i] = vram+i*0x1000;
		}
	}
	if( vmap_bytes[1] & 0x80 )
	{
		u32 mst = vmap_bytes[1] & 7;
		u32 ofs = (vmap_bytes[1]>>3)&3;
		if( mst == 1 || mst == 2 )
		{
			if( mst == 2 ) ofs &= 1;
			u8** temp = 0x20*ofs + ((mst==1) ? engineA_bg : engineA_obj);
			for(u32 i = 0; i < 32; ++i) temp[i] = vram+0x20000+i*0x1000;
		}
	}
	
	
	
	
	
	
	
	
}













