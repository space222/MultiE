#include <cstdlib>
#include <cstdio>
#include <SDL.h>
#include "Settings.h"
#include "a52.h"

extern console* sys;
#define A52 (*dynamic_cast<a52*>(sys))

void antic_reset();
void antic_scanline();
void ctia_write(u16, u8);
u8 ctia_read(u16);
void antic_write(u16, u8);
u8 antic_read(u16);
extern bool antic_wsync;

static u8 pregs[0x10] = {0};
static u8 pimask = 0;
bool once = false;
u8 incr = 0;

void try_kirq()
{
	auto keys = SDL_GetKeyboardState(nullptr);
	//if( !once )
	{
		once = true;
		//printf("incr was $%X\n", incr);
		pregs[9] = 0xc<<1; //0x19;
		if( pimask & 0x40 )
		{
			pregs[0xE] &= ~0x40;
			A52.cpu.irq_assert = true;
			//printf("IRQ!\n");
		}
	}
}

void a52::run_frame()
{	
	for(int line = 0; line < 262; ++line)
	{
		for(int i = 0; i < 114 && !antic_wsync; ++i) 
		{
			cpu.step();
		}
		antic_wsync = false;
		antic_scanline();
	}
}

void a52_write(u16 addr, u8 val)
{
	if( addr < 0x4000 ) 
	{
		((a52*)sys)->RAM[addr] = val;
		return;
	}
	if( addr >= 0xC000 && addr < 0xC100 )
	{
		ctia_write(addr, val);
		return;
	}
	if( addr >= 0xD400 && addr < 0xD500 ) 
	{
		antic_write(addr&0xf, val);
		return;
	}
	if( addr >= 0xE800 && addr < 0xF000 )
	{
		addr &= 0xF;
		if( addr == 0xE )
		{
			pimask = val;
			return;
		}
		pregs[addr] = val;
		if( addr == 0xb ) return;
		printf("POKEY $%X = $%X\n", addr, val);	
		return;
	}
}

u8 a52_read(u16 addr)
{
	if( addr < 0x4000 ) return ((a52*)sys)->RAM[addr];
	if( addr < 0xC000 ) return ((a52*)sys)->ROM[addr-0x4000];
	if( addr >= 0xD400 && addr < 0xD500 ) return antic_read(addr&0xf);
	if( addr >= 0xc000 && addr < 0xD000 ) return ctia_read(addr);
	if( addr >= 0xE800 && addr < 0xF000 )
	{
		addr &= 0xF;
		if( addr == 0xA ) return rand()&0xff;
		//printf("pokey read <$%X\n", addr);
		if( addr == 0xE )
		{
			u8 r = pregs[0xe];
			pregs[0xe] = 0xff;
			((a52*)sys)->cpu.irq_assert = false;
			//printf("IRQ cleared\n");
			return r;
		}
		return pregs[addr];
	}
	if( addr >= 0xF800 ) return ((a52*)sys)->BIOS[addr&0x7FF];
	//if( addr == 0xBFFF ) return 0x3F;
	//if( addr >= 0xBFE8 && addr <= 0xBFE8+0x13 ) return beans[addr-0xBFE8];
	//printf("R$%04X\n", addr);
	return 0;
}

a52::a52() : BIOS(2048)
{
	RAM.resize(16*1024);
	RAM[0x3FFD] = 0x4C;
	RAM[0x3FFE] = 0xFD;
	RAM[0x3FFF] = 0x3F;
	pregs[0xE] = 0xff;
	cpu.read = a52_read;
	cpu.write = a52_write;
}
	
bool a52::loadROM(std::string fname)
{
	std::string biosdir = Settings::get<std::string>("dirs", "firmware");
	std::string biosfile = Settings::get<std::string>("a52", "bios");
	if( biosfile.empty() ) biosfile = "5200.ROM";
	if( biosdir.empty() ) biosdir = "./bios/";
	if( biosdir.back() != '/' ) biosdir += '/';
	if( biosfile.find('/') == std::string::npos && biosfile.find('\\') == std::string::npos ) biosfile = biosdir + biosfile;
	
	FILE* fp = fopen(biosfile.c_str(), "rb");
	if( ! fp )
	{
		printf("unable to open bios (%s)\n", biosfile.c_str());
		return false;
	}
	[[maybe_unused]] int unu = fread(BIOS.data(), 1, 2048, fp);
	fclose(fp);
	
	//return true;
	fp = fopen(fname.c_str(), "rb");
	if( ! fp ) 
	{
		printf("unable to load rom\n");
		return false;
	}
	fseek(fp, 0, SEEK_END);
	auto fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ROM.resize(fsize<32*1024 ? 32*1024 : fsize);
	unu = fread(ROM.data(), 1, fsize, fp);
	if( fsize == 16*1024 )
	{
		for(int i = 0; i < 16*1024; ++i) ROM[16*1024+i] = ROM[i];	
	}
	fclose(fp);
	
	return true;

}

void a52::reset()
{
	cpu.reset();
	antic_reset();
}

