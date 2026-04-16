#include <print>
#include "nds.h"

void nds::run_frame()
{
	for(u32 i = 0; i < 1000; ++i)
	{
		arm7.step();
		arm9.step();
	}
	
	memcpy(fbuf, vram, 256*192*2);
}


void nds::reset()
{
	//arm7.reset();
	//arm9.reset();
}

bool nds::loadROM(std::string fname)
{
	arm7.read = [&](u32 a, int sz, ARM_CYCLE ct)-> u32 { return arm7_read(a, sz, ct); };
	arm7.write = [&](u32 a, u32 v, int sz, ARM_CYCLE ct) { arm7_write(a, v, sz, ct); };
	arm9.read = [&](u32 a, int sz, ARM_CYCLE ct)-> u32 { return arm9_read(a, sz, ct); };
	arm9.write = [&](u32 a, u32 v, int sz, ARM_CYCLE ct) { arm9_write(a, v, sz, ct); };
	arm9.inst_fetch = [&](u32 a, int sz, ARM_CYCLE ct) { return arm9_fetch(a, sz, ct); };

	FILE* fp = fopen(fname.c_str(), "rb");
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	
	u32 rom_offset = *(u32*)&ROM[0x20];
	arm9.r[15] = *(u32*)&ROM[0x24];
	arm9.flushp();
	u32 ram_addr = *(u32*)&ROM[0x28];
	u32 size = *(u32*)&ROM[0x2C];
	for(u32 i = 0; i < size; ++i, ++ram_addr, ++rom_offset)
	{
		mainram[ram_addr&(4_MB-1)] = ROM[rom_offset];
	}
	
	rom_offset = *(u32*)&ROM[0x30];
	arm7.r[15] = *(u32*)&ROM[0x34];
	arm7.flushp();
	ram_addr = *(u32*)&ROM[0x38];
	size = *(u32*)&ROM[0x3C];
	for(u32 i = 0; i < size; ++i, ++ram_addr, ++rom_offset)
	{
		mainram[ram_addr&(4_MB-1)] = ROM[rom_offset];
	}
	
	return true;
}

