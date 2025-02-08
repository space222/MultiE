#include <print>
#include <cstdio>
#include "bally_astrocade.h"
extern console* sys;

u8 bally_astrocade::read(u16 addr)
{
	if( addr < 0x2000 ) return bios[addr];
	if( addr < 0x4000 ) return ROM[addr-0x2000];
	return RAM[addr];
}

void bally_astrocade::write(u16 addr, u8 v)
{
	if( addr >= 0x4000 ) 
	{	
		RAM[addr] = v;
		return;
	}
	RAM[addr + 0x4000] = v;
}

u8 bally_astrocade::in(u16 addr)
{
	//std::println("Port in <${:X}", addr);
	return 0;
}

void bally_astrocade::out(u16 addr, u8 v)
{
	u8 P = addr&0xff;
	if( P == 0xD ) return;
	if( P == 0xC ) std::println("Port out ${:X} = ${:X}", addr, v);
}

bally_astrocade::bally_astrocade()
{
	cpu.read = [](u16 a) -> u8 { return dynamic_cast<bally_astrocade*>(sys)->read(a); };
	cpu.write= [](u16 a, u8 v) { dynamic_cast<bally_astrocade*>(sys)->write(a, v); };
	cpu.in = [](u16 a) -> u8 { return dynamic_cast<bally_astrocade*>(sys)->in(a); };
	cpu.out= [](u16 a, u8 v) { dynamic_cast<bally_astrocade*>(sys)->out(a, v); };
}

void bally_astrocade::run_frame()
{
	for(u32 i = 0; i < 300; ++i) cpu.step();
	
	for(u32 y = 0; y < 104; ++y)
	{
		for(u32 x = 0; x < 160; ++x)
		{
			u8 v = RAM[0x4000 + (y*40) + (x>>2)] >> (((x&3)^3)*2);
			v <<= 6;
			fbuf[y*160 + x] = (v<<24)|(v<<16)|(v<<8);
		}
	}
}

bool bally_astrocade::loadROM(const std::string fname)
{
	FILE* fp = fopen("./bios/astrocade.bios", "rb");
	if( !fp )
	{
		std::println("Unable to open ./bios/astrocade.bios");
		return false;
	}
	[[maybe_unused]] int u = fread(bios, 1, 0x2000, fp);
	fclose(fp);

	fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		std::println("Unable to open '{}'", fname);
		return false;
	}
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	u = fread(ROM, 1, fsz>0x4000?0x4000:fsz, fp);
	fclose(fp);
	
	return true;
}



