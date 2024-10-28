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

u32 gba::read(u32 addr, int size, ARM_CYCLE)
{
	if( addr >= 0x0800'0000u )
	{
		addr &= 0x01ffFFFF;
		return sized_read(ROM.data(), addr%ROM.size(), size);
	}
	printf("GBA: read%i <$%X\n", size, addr);
	return 0;
}

void gba::write(u32 addr, u32 v, int size, ARM_CYCLE)
{
	printf("GBA: Write%i $%X = $%X\n", size, addr, v);
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
	for(u32 i = 0; i < 25; ++i ) cpu.step();
	exit(1);
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



