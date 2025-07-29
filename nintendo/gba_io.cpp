#include "gba.h"

static u16 snd88 = 0;

void gba::write_io(u32 addr, u32 v, int size)
{
	if( size == 8 )
	{
		v &= 0xff;
		if( addr == 0x04000301 )
		{
			halted = true;
			if( !IME ) { std::println("Halted with IME=0"); exit(1); }
			return;
		}
		if( addr == 0x04000202 )
		{
			ISTAT = (ISTAT&0xff00) | (ISTAT & ~v);
			check_irqs();
			return;
		}
		if( addr == 0x04000203 ) 
		{
			ISTAT = (ISTAT&0x00ff) | (ISTAT & ~(v<<8));
			check_irqs();
			return;
		}
		u16 c = read_io(addr&~1, 16);
		c &= 0xff00>>((addr&1)*8);
		c |= v<<((addr&1)*8);
		write_io(addr&~1, c, 16);
		return;
	}
	if( size == 32 )
	{
		write_io(addr, v&0xffff, 16);
		write_io(addr+2, (v>>16)&0xffff, 16);
		return;
	}
	
	if( addr == 0x04000088 ) { snd88 = v; return; }

	if( addr < 0x04000060 ) { write_lcd_io(addr, v); return; }
	if( addr < 0x040000B0 ) { write_snd_io(addr, v); return; }
	if( addr < 0x04000100 ) { write_dma_io(addr, v); return; }
	if( addr < 0x04000120 ) { write_tmr_io(addr, v); return; }
	if( addr < 0x04000130 ) { write_comm_io(addr, v); return; }
	if( addr < 0x04000134 ) { write_pad_io(addr, v); return; }
	if( addr < 0x04000200 ) { write_comm_io(addr, v); return; }
	if( addr < 0x04000800 ) 
	{
		
		write_sys_io(addr, v); 
		return;
	}
	if( (addr & 0xFFFC) == 0x0800 ) { write_memctrl_io(addr, v); return; }
	
	std::println("write_io{} error: ${:X} = ${:X}", size, addr, v);
	//exit(1);
	return;
}

u32 gba::read_io(u32 addr, int size)
{
	if( size == 8 )
	{
		u16 v = read_io(addr&~1, 16);
		return (v>>((addr&1)*8)) & 0xff;
	}
	if( size == 32 )
	{
		u32 v = read_io(addr, 16);
		v |= read_io(addr+2, 16)<<16;
		return v;
	}
	
	if( addr == 0x04000088 ) { return snd88; }
	if( addr == 0x04000130 ) { return getKeys(); }

	if( addr < 0x04000060 ) return read_lcd_io(addr);
	if( addr < 0x040000B0 ) return read_snd_io(addr);
	if( addr < 0x04000100 ) return read_dma_io(addr);
	if( addr < 0x04000120 ) { std::println("read timr reg ${:X}!", addr); return read_tmr_io(addr); }
	if( addr < 0x04000130 ) return read_comm_io(addr);
	if( addr < 0x04000134 ) return read_pad_io(addr);
	if( addr < 0x04000200 ) return read_comm_io(addr);
	if( addr < 0x04000800 ) return read_sys_io(addr);
	if( (addr & 0xFFFC) == 0x0800 ) return read_memctrl_io(addr);
	
	std::println("read_io error: addr = ${:X}", addr);
	//exit(1);
	return 0; //todo: this should work, but return garbage like prefetch or open bus
}

u32 gba::read_sys_io(u32 addr)
{
	switch( addr )
	{
	case 0x04000200: return IMASK;
	case 0x04000202: return ISTAT;
	case 0x04000208: return IME;
	default: break;
	}
	return 0;
}

void gba::write_sys_io(u32 addr, u32 v)
{
	switch( addr )
	{
	case 0x04000200: IMASK = v&0x3fff; break;
	case 0x04000202: ISTAT &= ~(v&0x3fff); break;
	case 0x04000208: IME = v&1; break;
	default: break;
	}
	//cpu.irq_line = IME && (ISTAT & IMASK & 0x3fff);
	check_irqs();
}


