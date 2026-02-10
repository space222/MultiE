#include <print>
#include <vector>
#include <iostream>
#include "trs80cc3.h"

u8 trs80cc3::read_io(u8 a)
{
	switch( a )
	{
	case 0x00: return 0xff;
//	case 0x02: return 0xff;
	
	default: break;
	}
	std::println("${:X}: IO Rd ${:X}", cpu.pc, a);
	return io[a];
}

void trs80cc3::write_io(u8 a, u8 v)
{
	std::println("${:X}: IO Wr ${:X} = ${:X}", cpu.pc, a, v);
	switch( a )
	{
	case 0xA0:
	case 0xA1:
	case 0xA2:
	case 0xA3:
	case 0xA4:
	case 0xA5:
	case 0xA6:
	case 0xA7:
	case 0xA8:
	case 0xA9:
	case 0xAA:
	case 0xAB:
	case 0xAC:
	case 0xAD:
	case 0xAE:
	case 0xAF:
		v &= 0x3F;
		if( v >= 0x3c )
		{// ??
			v &= ~3;
			v |= a&3;
		}
		io[a] = v;
		return;
	case 0xDE: rom_enabled = true; return;
	case 0xDF: rom_enabled = false; return;
	default: break;
	}
	io[a] = v;
}

#define MMU_ENABLED (io[0x90]&BIT(6))
#define RAM_VECTORS (io[0x90]&BIT(3))

u8 trs80cc3::read(u16 a)
{
	if( a >= 0xff00 ) return read_io(a&0xff);
	if( a >= 0xfe00 && (!MMU_ENABLED || RAM_VECTORS) ) return ram[0x7fe00 | (a&0xff)];
	u32 phys_index = (MMU_ENABLED ? io[0xA0 + (a>>13) + ((io[0x91]&1)<<4)] : (0x38+(a>>13)));
	u32 paddr = (phys_index<<13)|(a&0x1fff);
	if( rom_enabled && paddr >= 0x78000 && paddr < 0x7fe00 ) return bios[paddr&0x7fff];
	return ram[paddr];
}

void trs80cc3::write(u16 a, u8 v)
{
	if( a >= 0xff00 ) return write_io(a&0xff, v);
	if( a >= 0xfe00 && (!MMU_ENABLED || RAM_VECTORS) ) { ram[0x7fe00 | (a&0xff)] = v; return; }
	u32 phys_index = (MMU_ENABLED ? io[0xA0 + (a>>13) + ((io[0x91]&1)<<4)] : (0x38+(a>>13)));
	u32 paddr = (phys_index<<13)|(a&0x1fff);
	if( rom_enabled && paddr >= 0x78000 && paddr < 0x7fe00 ) { return; }
	ram[paddr] = v;
}

static u32 cc3pal[] = { 0, 0xff0000, 0x00ff00, 0x00ffff, 0xffff00, 0xf0f0f0, 0xf00f0f, 0x135846, 
		0xff, 0xff0880, 0x88ff00, 0x088fff, 0xff8fc, 0x20f6f0, 0xf0020f, 0x122840 };

void trs80cc3::run_frame()
{
	u64 target = last_target + 29833;
	while( stamp < target )
	{
		//if( cpu.pc >= 0xA000 && cpu.pc < 0xb000 ) { char a; std::cin >> a; }
		stamp += cpu.step();
	}
	last_target = target;
	
	for(u32 y = 0; y < 192; ++y)
	{
		for(u32 x = 0; x < 256; ++x)
		{
			u8 b = ram[0x70e00 + y*32 + (x/8)];
			b >>= (x&7)^7;
			fbuf[y*256 + x] = (b&1)?0xffFFff:0;
		}
	}
}

void trs80cc3::reset()
{
	cpu.reset( __builtin_bswap16(*(u16*)&bios[0x7ffe]) );
	cpu.reader = [&](u16 a)->u8 { return read(a); };
	cpu.writer = [&](u16 a, u8 v) { write(a,v); };
	memset(io, 0, 0x100);
	for(u32 i = 0; i < 8; ++i) { io[0xA0+i] = io[0xA8+i] = 0x38 + i; }
}

bool trs80cc3::loadROM(std::string fname)
{
	if( !freadall(bios, fopen("./bios/trs80cc3.bios", "rb"), 32_KB) )
	{
		std::println("Need a TRS-80 CoCo3 BIOS");
		return false;
	}
	return true;
}


