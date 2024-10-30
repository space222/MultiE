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

static void sized_write(u8* data, u32 index, u32 v, int size)
{
	if( size == 8 ) { data[index] = v; return; }
	if( size == 16 ) { *(u16*)&data[index] = v; return; }
	*(u32*)&data[index] = v;
}

u16 dispstat = 0;

u32 gba::read(u32 addr, int size, ARM_CYCLE ct)
{
	if( addr <= 0x1000 )
	{
		printf("Attempt to read BIOS. dying for now\n");
		cpu.dump_regs();
		exit(1);
		return 0;
	}
	if( addr == 0x0400'0004u )
	{
		printf("DISPSTAT Read\n");
		u16 r = dispstat;
		dispstat ^= 1;
		return r;
	}
	if( addr == 0x4000130u )
	{
		return 0x3ff;	
	}
	if( addr >= 0x0800'0000u )
	{
		cpu.icycles += 5;
		addr &= 0x01ffFFFF;
		return sized_read(ROM.data(), addr%ROM.size(), size);
	}
	if( addr >= 0x0200'0000u && addr < 0x0300'0000 )
	{
		cpu.icycles += ((size==32) ? 6 : 3);
		addr &= 0x3FFFF;
		return sized_read(ewram, addr, size);
	}
	if( addr >= 0x0300'0000u && addr < 0x0400'0000 )
	{
		cpu.icycles += 1;
		addr &= 0x7FFF;
		return sized_read(iwram, addr, size);
	}
	printf("GBA: read%i <$%X\n", size, addr);
	if( addr >= 0x0600'0000 && addr < 0x0700'0000 )
	{
		cpu.icycles += ((size==32)?2:1);
		addr &= 0x3ffff;
		if( size == 8 ) { return vram[addr]; }
		if( size == 16 ) { return *(u16*)&vram[addr]; }
		return *(u32*)&vram[addr];
	}
	if( addr >= 0x0500'0000 && addr < 0x0600'0000 )
	{
		cpu.icycles += ((size==32)?2:1);
		addr &= 0x3ff;
		return sized_read(palette, addr, size);
	}
	cpu.icycles += 1;
	return 0;
}

void gba::write(u32 addr, u32 v, int size, ARM_CYCLE)
{
	printf("GBA: Write%i $%X = $%X\n", size, addr, v);
	if( addr >= 0x0600'0000 && addr < 0x0700'0000 )
	{
		cpu.icycles += ((size==32)?2:1);
		addr &= 0x3ffff;
		if( size == 8 ) { v = (v<<8)|v; size = 16; }
		if( size == 16 ) { *(u16*)&vram[addr] = v; }
		else if( size == 32 ) { *(u32*)&vram[addr] = v; }
		return;
	}
	if( addr >= 0x0500'0000 && addr < 0x0600'0000 )
	{
		cpu.icycles += ((size==32)?2:1);
		addr &= 0x3ff;
		if( size == 8 ) { v = (v<<8)|v; size = 16; }
		if( size == 16 ) { *(u16*)&palette[addr] = v; }
		else if( size == 32 ) { *(u32*)&palette[addr] = v; }
		return;
	}
	if( addr >= 0x0200'0000u && addr < 0x0300'0000 )
	{
		cpu.icycles += ((size==32) ? 6 : 3);
		addr &= 0x3FFFF;
		sized_write(ewram, addr, v, size);
		return;
	}
	if( addr >= 0x0300'0000u && addr < 0x0400'0000 )
	{
		cpu.icycles += 1;
		addr &= 0x7FFF;
		sized_write(iwram, addr, v, size);
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
	//memcpy(fbuf, vram, 240*160*2);
	for(u32 y = 0; y < 160; ++y)
	{
		for(u32 x = 0; x < 240; ++x)
		{
			fbuf[y*240+x] = *(u16*)&palette[vram[y*240+x]<<1];		
		}	
	}
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



