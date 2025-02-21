#include <print>
#include <vector>
#include <string>
#include "Lynx.h"

u8 Lynx::read(u16 addr)
{
	if( addr >= 0xfe00 ) return bios[addr&0x1ff];
	if( addr >= 0xfc00 )
	{
		std::println("<${:X}", addr);
	}
	return RAM[addr];
}

void Lynx::write(u16 addr, u8 v)
{
	if( addr >= 0xfc00 )
	{
		std::println("${:X} = ${:X}", addr, v);
	}
	RAM[addr] = v;
}

void Lynx::run_frame()
{
	for(u32 i = 0; i < 66666; ++i)
	{
		cycle();
	}
}

void Lynx::reset()
{
	cpu.reader = [&](coru6502&, u16 a)->u8{ return read(a); };
	cpu.writer = [&](coru6502&, u16 a, u8 v) { write(a,v); };
	cycle = cpu.run();
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
	return true;
}


