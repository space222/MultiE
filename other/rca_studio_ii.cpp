#include <cstdio>
#include <print>
#include "rca_studio_ii.h"

u8 rca_studio_ii::read(u16 addr)
{
	if( addr < 0x800 ) return bios[addr];
	if( addr < 0xA00 ) return ram[addr&0x1ff];
	std::println("read ${:X}", addr);
	return 0;
}

void rca_studio_ii::write(u16 addr, u8 v)
{
	if( addr >= 0x800 && addr < 0xA00 ) { ram[addr&0x1ff] = v; return; }
	std::println("write ${:X} = ${:X}", addr, v);
}

void rca_studio_ii::run_frame()
{
	for(u32 i = 0; i < 500; ++i ) cpu.step();
	for(u32 y = 0; y < 32; ++y)
	{
		for(u32 x = 0; x < 64; ++x)
		{
			u8 b = ram[0x100 + (y*8) + (x>>3)];
			b >>= (x&7);
			b &= 1;
			fbuf[y*64+x] = (b ? 0xffffff00 : 0);	
		}
	}
}

bool rca_studio_ii::loadROM(const std::string fname)
{
	FILE* fp = fopen("./bios/rca_studio_ii.bios", "rb");
	if(!fp)
	{
		std::println("Unable to open bios/rca_studio_ii.bios");
		return false;
	}
	[[maybe_unused]] int unu = fread(bios, 1, 2*1024, fp);
	fclose(fp);
	
	fp = fopen(fname.c_str(), "rb");
	if(!fp)
	{
		std::println("Unable to open <{}>", fname);
		return false;
	}
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ROM.resize(fsz);
	unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	return true;
}

void rca_studio_ii::reset()
{
	cpu.read = [&](u16 addr) -> u8 { return read(addr); };
	cpu.write = [&](u16 addr, u8 v) { write(addr, v); };
	cpu.out = [&](u16 a, int b) { std::println("out {} {}", a, b); };
	cpu.in = [&](u16) -> u8 { printf("in\n"); return 0; };
	cpu.reset();
}


