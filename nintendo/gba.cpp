#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include "gba.h"

const u32 cycles_per_frame = 280896;

void gba::write(u32 addr, u32 v, int size, ARM_CYCLE ct)
{
	
} //end of write

u32 gba::read(u32 addr, int size, ARM_CYCLE ct)
{
	return 0;
}

void gba::reset()
{
	cpu.read = [&](u32 a, int s, ARM_CYCLE c) -> u32 { return read(a,s,c); };
	cpu.write= [&](u32 a, u32 v, int s, ARM_CYCLE c) { write(a,v,s,c); };
	cpu.reset();
}

void gba::run_frame()
{
	
	
}

bool gba::loadROM(const std::string fname)
{
	{
		FILE* fbios = fopen("./bios/GBABIOS.BIN", "rb");
		if( !fbios )
		{
			printf("Need gba.bios in the bios folder\n");
			return false;
		}
		[[maybe_unused]] int unu = fread(bios, 1, 16*1024, fbios);
		fclose(fbios);
	}

	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		return false;
	}

	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	return true;	
}

u16 gba::getKeys()
{
	auto keys = SDL_GetKeyboardState(nullptr);
	u16 val = 0x3ff;
	if( keys[SDL_SCANCODE_Q] ) val ^= BIT(9);
	if( keys[SDL_SCANCODE_W] ) val ^= BIT(8);
	if( keys[SDL_SCANCODE_DOWN] ) val ^= BIT(7);
	if( keys[SDL_SCANCODE_UP] ) val ^= BIT(6);
	if( keys[SDL_SCANCODE_LEFT] ) val ^= BIT(5);
	if( keys[SDL_SCANCODE_RIGHT] ) val ^= BIT(4);
	if( keys[SDL_SCANCODE_S] ) val ^= BIT(3);
	if( keys[SDL_SCANCODE_A] ) val ^= BIT(2);
	if( keys[SDL_SCANCODE_X] ) val ^= BIT(1);
	if( keys[SDL_SCANCODE_Z] ) val ^= BIT(0);
	return val;
}


