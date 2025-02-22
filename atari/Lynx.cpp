#include <print>
#include <vector>
#include <string>
#include "Lynx.h"

#define SUZYHREV 0xFC88
#define IODAT    0xFD8B
#define SYSCTL1  0xFD87
#define RCART    0xFCB2

u8 Lynx::mikey_read(u16 addr)
{
	std::println("Mikey <${:X}", addr);
	return 0;
}

u8 Lynx::suzy_read(u16 addr)
{
	if( addr == SUZYHREV )
	{ // hardware version
		return 1;
	}
	if( addr == RCART )
	{
		u8 r = rom[(cart_block<<cart_shift)|(cart_offset&cart_mask)];
		std::println("cart read [${:X}:${:X}] got ${:X}", cart_block, cart_offset, r);
		cart_offset += 1;
		return r;
	}
	std::println("Suzy <${:X}", addr);
	return 0;
}

void Lynx::mikey_write(u16 addr, u8 v)
{
	if( addr == IODAT )
	{
		cart_data = v;
		return;
	}
	if( addr == SYSCTL1 )
	{
		if( !(cart_strobe&1) && (v&1) )
		{
			cart_block <<= 1;
			cart_block |= (cart_data>>1)&1;	
		}
		if( !(v&1) )
		{
			cart_offset = 0;
		}
		cart_strobe = v;
		return;	
	}
	std::println("Mikey ${:X} = ${:X}", addr, v);
}

void Lynx::suzy_write(u16 addr, u8 v)
{
	std::println("Suzy ${:X} = ${:X}", addr, v);
}

u8 Lynx::read(u16 addr)
{
	if( addr >= 0xfffa || addr == 0xfff8 )
	{
		if( mmctrl_vectors() ) return bios[addr&0x1ff];
		return RAM[addr];
	}
	if( addr == 0xfff9 )
	{
		return mmctrl;
	}
	if( addr >= 0xfe00 )
	{
		if( mmctrl_rom() ) return bios[addr&0x1ff];
		return RAM[addr];
	}
	if( addr >= 0xfd00 )
	{
		if( mmctrl_mikey() ) return mikey_read(addr);
		return RAM[addr];
	}
	if( addr >= 0xfc00 )
	{
		if( mmctrl_suzy() ) return suzy_read(addr);
		return RAM[addr];
	}
	return RAM[addr];
}

void Lynx::write(u16 addr, u8 v)
{
	if( addr >= 0xfffa || addr == 0xfff8 )
	{
		if( !mmctrl_vectors() ) RAM[addr] = v;
		return;
	}
	if( addr == 0xfff9 )
	{
		mmctrl = v;
		return;
	}
	if( addr >= 0xfe00 )
	{
		if( !mmctrl_rom() ) RAM[addr] = v;
		return;
	}
	if( addr >= 0xfd00 )
	{
		if( mmctrl_mikey() )
		{
			mikey_write(addr, v);
		} else {
			RAM[addr] = v;
		}
		return;
	}
	if( addr >= 0xfc00 )
	{
		if( mmctrl_suzy() )
		{
			suzy_write(addr, v);
		} else {
			RAM[addr] = v;
		}
		return;
	}
	RAM[addr] = v;
}

void Lynx::run_frame()
{
	for(u32 i = 0; i < 3000; ++i)
	{
		//std::println("${:X}", cpu.pc);
		cycle();
	}
}

void Lynx::reset()
{
	mmctrl = 0;
	cpu.reader = [&](coru6502&, u16 a)->u8{ return read(a); };
	cpu.writer = [&](coru6502&, u16 a, u8 v) { write(a,v); };
	cpu.cpu_type = CPU_65C02;
	cycle = cpu.run();
	for(u32 i = 0; i < 0x10000; ++i) RAM[i] = 0xff;
}

bool Lynx::loadROM(const std::string fname)
{
	FILE* fp = fopen("./bios/lynxboot.img", "rb");
	if( !fp )
	{
		std::println("Need lynxboot.img in ./bios");
		return false;
	}
	[[maybe_unused]] int unu = fread(bios, 1, 512, fp);
	fclose(fp);
	
	fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		std::println("Unable to open <{}>", fname);
		return false;
	}
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	rom.resize(fsz);
	unu = fread(rom.data(), 1, fsz, fp);
	fclose(fp);
	if( rom[0] == 'L' && rom[1] == 'Y' && rom[2] == 'N' && rom[3] == 'X' )
	{
		std::println("Lynx ROM has unsupported header, will ignore it");
		rom.erase(rom.begin(), rom.begin()+64);
	}
	fsz -= 64;
	if( fsz > 128*1024 ) 
	{
		cart_mask = 0x3ff;
		cart_shift = 10;
	} else if( fsz > 64*1024 ) {
		cart_mask = 0x1ff;
		cart_shift = 9;
	} else {
		cart_mask = 0xff;
		cart_shift = 8;
	}
	return true;
}


