#include <print>
#include "util.h"
#include "pokemini.h"

void pokemini::run_frame()
{
	for(u32 i = 0; i < 1000; ++i)
	{
		cpu.step();
	}
	
	
	for(u32 y = 0; y < 64; ++y)
	{
		for(u32 x = 0; x < 96; ++x)
		{
			fbuf[y*96 + x] = ((RAM[y*12 + (x/8)] & BIT(x&7)) ? 0xffFFff00 : 0);
		}
	}
}

void pokemini::reset()
{
	cpu.reset();
}

void pokemini::io_write(u32 a, u8 v)
{
	std::println("IO Wr ${:X} = ${:X}", a, v);
}

u8 pokemini::io_read(u32 a)
{
	std::println("IO Rd ${:X}", a);
	if( a == 0x2027 ) return 0x40;
	return 0;
}

void pokemini::write(u32 a, u8 v)
{
	a &= 2_MB-1;
	if( a >= 0x1000 && a < 0x2000 ) { RAM[a&0xfff] = v; return; }
	if( a >= 0x2000 && a < 0x2100 ) { io_write(a, v); return; }
	std::println("Write to ROM region ${:X} = ${:X}", a, v);
}

u8 pokemini::read(u32 a)
{
	a &= 2_MB-1;
	if( a < 0x1000 ) return bios[a];
	if( a < 0x2000 ) return RAM[a&0xfff];
	if( a < 0x2100 ) return io_read(a);
	return rom[a%rom.size()];
}

bool pokemini::loadROM(std::string fname)
{
	FILE* fp = fopen("./bios/pokemini.bios", "rb");
	if( !fp )
	{
		std::println("need ./bios/pokemini.bios");
		return false;	
	}
	[[maybe_unused]] int unu = fread(bios, 1, 4_KB, fp);
	fclose(fp);
	
	if( !freadall(rom, fopen(fname.c_str(), "rb")) )
	{
		std::println("Unable to open '{}'", fname);
		return false;
	}
	
	cpu.read = [&](u32 a)->u8 { return read(a); };
	cpu.write = [&](u32 a,u8 v) { write(a, v); };
	
	return true;
}

