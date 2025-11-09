#include <print>
#include <cstdio>
#include "genesis.h"
#include "util.h"

u32 genesis::sh2master_read(u32 addr, int size)
{
	if( addr < 0x4000 ) { return sized_read_be(bios32xM, addr&0x7ff, size); }
	if( addr >= 0xF0000000u )
	{
		std::println("sh2m internal reg rd ${:X}", addr);
		exit(1);
		return 0;
	}
	if( addr >= 0xC0000000u )
	{
		std::println("sh2m cache scratchpad rd ${:X}", addr);
		exit(1);
		return 0;
	}
	addr &= ~0x20000000;
	if( addr >= 0x6000000 && addr < 0x6040000 ) { return sized_read_be(sdram, addr&0x3ffff, size); }
	if( addr >= 0x2000000 && addr < 0x2400000 ) { return sized_read_be(ROM.data(), addr&0x3Fffff, size); }

	if( addr >= 0x4200 && addr < 0x4400 ) { return sized_read_be(pal32x, addr&0x1ff, size); }
	if( addr >= 0x4020 && addr < 0x4030 ) { return sized_read_be(comms, addr&0xf, size); }
	if( addr == 0x4000 ) 
	{ 
		if( size == 32 ) { std::println("got 32bit read of io"); exit(1); }
		u16 r = intctrlM|((adapter_ctrl&2)<<8);
		if( size == 8 ) return r>>8;
		return r; 
	}
	
	if( addr == 0x410A || addr == 0x410B ) return fbctrl;

	std::println("SH2m: read{} ${:X}", size, addr);
	exit(1);
	return 0xcafe;
}

u32 genesis::sh2slave_read(u32 addr, int size)
{
	if( addr < 0x4000 ) { return sized_read_be(bios32xS, addr&0x7ff, size); }
	if( addr >= 0xF0000000u )
	{
		std::println("sh2s internal reg rd ${:X}", addr);
		exit(1);
		return 0;
	}
	if( addr >= 0xC0000000u )
	{
		std::println("sh2s cache scratchpad rd ${:X}", addr);
		exit(1);
		return 0;
	}
	addr &= ~0x20000000;
	if( addr >= 0x6000000 && addr < 0x6040000 ) { return sized_read_be(sdram, addr&0x3ffff, size); }
	if( addr >= 0x2000000 && addr < 0x2400000 ) { return sized_read_be(ROM.data(), addr&0x3Fffff, size); }

	if( addr >= 0x4200 && addr < 0x4400 ) { return sized_read_be(pal32x, addr&0x1ff, size); }
	if( addr >= 0x4020 && addr < 0x4030 ) { return sized_read_be(comms, addr&0xf, size); }
	if( addr == 0x4000 ) 
	{ 
		if( size == 32 ) { std::println("got 32bit read of io"); exit(1); }
		u16 r = intctrlS|((adapter_ctrl&2)<<8);
		if( size == 8 ) return r>>8;
		return r;
	}
	
	std::println("SH2s: read{} ${:X}", size, addr);
	exit(1);
	return 0xbabe;
}

void genesis::sh2master_write(u32 addr, u32 val, int size)
{
	if( addr >= 0xF0000000u )
	{
		std::println("SH2m: internal write{} ${:X}= ${:X}", size, addr, val);
		return;
	}
	if( addr >= 0xC0000000u )
	{
		std::println("SH2m: cache scratchpad write?{} ${:X} = ${:X}", size, addr, val);
		return;
	}
	addr &= ~0x20000000;
	if( addr >= 0x6000000 && addr < 0x6040000 ) { sized_write_be(sdram, addr&0x3ffff, val, size); return; }
	if( addr >= 0x4020 && addr < 0x4030 ) { sized_write_be(comms, addr&0xf, val, size); }

	std::println("SH2m: write{} ${:X} = ${:X}", size, addr, val);
}

void genesis::sh2slave_write(u32 addr, u32 val, int size)
{
	if( addr >= 0xF0000000u )
	{
		std::println("SH2m: internal write{} ${:X}= ${:X}", size, addr, val);
		return;
	}
	if( addr >= 0xC0000000u )
	{
		std::println("SH2m: cache scratchpad write?{} ${:X} = ${:X}", size, addr, val);
		return;
	}
	addr &= ~0x20000000;
	if( addr >= 0x6000000 && addr < 0x6040000 ) { sized_write_be(sdram, addr&0x3ffff, val, size); return; }
	if( addr >= 0x4020 && addr < 0x4030 ) { sized_write_be(comms, addr&0xf, val, size); return; }
	
	std::println("SH2s: write{} ${:X} = ${:X}", size, addr, val);
}

u32 genesis::read32x(u32 addr, int size)
{
	addr &= 0xffFFff;
	if( addr < 0x100 ) return sized_read_be(vecrom, addr, size);
	if( addr >= 0xFF0000 ) return sized_read_be(RAM, addr&0xffff, size);

	if( addr >= 0x880000 && addr < 0x900000 ) return sized_read_be(ROM.data(), addr-0x880000, size);

	std::println("32X-68k:${:X}: read{} <${:X}", cpu.pc-2 , size, addr);

	if( addr == 0xA15180 ) return bmp_mode|0x8000; //todo: bit15 is 1=NTSC/0=PAL

	if( addr == 0xA1518A ) return fbctrl|current_frame;
	if( addr >= 0xA15200 && addr < 0xA15400 ) return sized_read_be(pal32x, addr&0x1ff, size);
	if( addr >= 0xA15120 && addr < 0xA15130 ) 
	{
		u16 r = sized_read_be(comms, addr&0xf, size);
		std::println("68k: rd comms[${:X}] = ${:X}", addr&0xf, r);
		return r;
	}

	if( addr == 0xA10000 ) return (pal ? BIT(6):0)|(domestic ?0:BIT(7)); //(pal ? 0xc0 : 0x80);
	if( addr == 0xA1000C ) return 0;
	
	if( addr == 0xA10008 ) return pad1_ctrl;
	if( addr == 0xA1000A ) return pad2_ctrl;
	if( addr == 0xA10002 )
	{
		return getpad1();
	}
	
	if( addr == 0xA10004 ) return getpad2();
	
	if( addr == 0xC0'0004 || addr == 0xC0'0006 ) 
	{
		//printf("VDP stat\n");
		vdp_latch = false;
		return vdp_stat;
	}
	
	if( addr == 0xC0'0000 || addr == 0xC0'0002 )
	{
		vdp_latch = false;
		return vdp_read();
	}
	
	if( addr == 0xC00008 )
	{
		std::println("HV counter read");
		//exit(1);
		return 0;
	}
	
	if( addr == 0xA11100 )
	{
		return z80_busreq;
	}
	
	if( addr == 0xA11200 )
	{
		return z80_reset;
	}
	
	if( addr == 0xA130EC ) return ('M'<<8)|'A';
	if( addr == 0xA130EE ) return ('R'<<8)|'S';

	if( addr < 0x400000 )
	{
		exit(1);
		return sized_read_be(ROM.data(), addr, size);
	}
	
	return 0;
}

void genesis::write32x(u32 addr, u32 val, int size)
{
	if( addr < 0x100 )
	{
		std::println("32X-68k: write to vector rom space {}bit ${:X} = ${:X}", size, addr, val);
		return;
	}
	std::println("32X-68k: Write{} ${:X} = ${:X}", size, addr, val);
	
	if( addr == 0xA15100 || addr == 0xA15101 )
	{
		adapter_ctrl = (adapter_ctrl&3)|val;
		return;
	}

	if( addr >= 0xA15200 && addr < 0xA15400 ) return sized_write_be(pal32x, addr&0x1ff, val, size);
	if( addr >= 0xA15120 && addr < 0xA15130 ) return sized_write_be(comms, addr&0xf, val, size);
	
	if( addr == 0xA15180 ) { bmp_mode = val&0x7fff; return; }
	if( addr == 0xA1518A || addr == 0xA1518B ) 
	{
		next_frame = val&1;
		if( (fbctrl & 0x8000) || !(bmp_mode&3) )
		{
			current_frame = next_frame;
		}
		return;
	}
	
	
	return;
}


