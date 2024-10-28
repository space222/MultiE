#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include "gba.h"
extern console* sys;
const u32 cycles_per_frame = 280896;

static u32 sized_read(u8* data, u32 index, int size)
{
	if( size == 8 ) return data[index];
	if( size == 16 ) return *(u16*)&data[index];
	return *(u32*)&data[index];
}

u32 gba::read(u32 addr, int size, ARM_CYCLE ct)
{
	if( addr >= 0x0800'0000u )
	{
		cpu.icycles += 5;
		addr &= 0x01ffFFFF;
		return sized_read(ROM.data(), addr%ROM.size(), size);
	}
	printf("GBA: read%i <$%X\n", size, addr);
	if( addr >= 0x0600'0000 && addr <= 0x0700'0000 )
	{
		cpu.icycles += ((size==32)?2:1);
		addr &= 0x3ffff;
		if( size == 8 ) { return vram[addr]; }
		if( size == 16 ) { return *(u16*)&vram[addr]; }
		return *(u32*)&vram[addr];
	}
	return 0;
}

void gba::write(u32 addr, u32 v, int size, ARM_CYCLE)
{
	printf("GBA: Write%i $%X = $%X\n", size, addr, v);
	if( addr >= 0x0600'0000 && addr <= 0x0700'0000 )
	{
		cpu.icycles += ((size==32)?2:1);
		addr &= 0x3ffff;
		if( size == 8 ) { v = (v<<8)|v; size = 16; }
		if( size == 16 ) { *(u16*)&vram[addr] = v; }
		else if( size == 32 ) { *(u32*)&vram[addr] = v; }
		return;
	}
}


void gba::reset()
{
	cpu.cpsr.v = 0;
	cpu.cpsr.b.M = ARM_MODE_USER;
	cpu.cpsr.b.I = 1;
	cpu.cpsr.b.F = 1;
	cpu.r[15] = 0x0800'0000u;
	cpu.r[13] = 0x0300'7F00u;
	cpu.read = [](u32 a, int s, ARM_CYCLE c) -> u32 { return dynamic_cast<gba*>(sys)->read(a,s,c); };
	cpu.write=[](u32 a, u32 v, int s, ARM_CYCLE c) { dynamic_cast<gba*>(sys)->write(a,v,s,c); };

	cpu.reset();
}

void gba::run_frame()
{
	u64 stamp = 0;
	while( stamp < cycles_per_frame )
	{
		stamp += cpu.step();
	}
	memcpy(fbuf, vram, 240*160*2);
}

bool gba::loadROM(const std::string fname)
{
	FILE* fbios = fopen("./bios/GBABIOS.BIN", "rb");
	if( !fbios )
	{
		printf("Need gba.bios in the bios folder\n");
		return false;
	}
	[[maybe_unused]] int unu = fread(bios, 1, 16*1024, fbios);
	fclose(fbios);
	

	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
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



