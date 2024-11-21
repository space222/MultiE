#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include "gba.h"
extern console* sys;
const u32 cycles_per_frame = 280896;
u32 dispstat = 0;

void gba::write(u32 addr, u32 v, int size, ARM_CYCLE ct)
{
	cpu.icycles += 1;
	//printf("GBA: Write%i $%X = $%X\n", size, addr, v);
	if( addr < 0x0200'0000 )
	{
		printf("$%X: Attempt to write BIOS\n", cpu.r[15]-(cpu.cpsr.b.T?4:8));
		exit(1);
	}
	if( addr < 0x0300'0000 )
	{
		addr &= 0x3ffff;
		if( size == 8 ) { ewram[addr] = v; return; }
		if( size == 16 ) { *(u16*)&ewram[addr&~1] = v; return; }
		*(u32*)&ewram[addr&~3] = v; return;	
	}
	if( addr < 0x0400'0000 )
	{
		addr &= 0x7fff;
		if( size == 8 ) { iwram[addr] = v; return; }
		if( size == 16 ) { *(u16*)&iwram[addr&~1] = v; return; }
		*(u32*)&iwram[addr&~3] = v; return;	
	}
	if( addr < 0x0500'0000 )
	{
		printf("IO write $%X = $%X\n", addr, v);
		return;	
	}
	if( addr < 0x0600'0000 )
	{
		addr &= 0x3ff;
		if( size == 8 ) { v = (v<<8)|v; size = 16; }
		if( size == 16 ) { *(u16*)&palette[addr&~1] = v; return; }
		*(u32*)&palette[addr&~3] = v; return;	
	}
	if( addr < 0x0700'0000 )
	{
		addr &= 0x1ffff;
		if( size == 8 ) { v = (v<<8)|v; size = 16; }
		if( size == 16 ) { *(u16*)&vram[addr&~1] = v; return; }
		*(u32*)&vram[addr&~3] = v; return;	
	}
	if( addr < 0x0700'0000 )
	{
		addr &= 0x3ff;
		if( size == 8 ) { v = (v<<8)|v; size = 16; }
		if( size == 16 ) { *(u16*)&oam[addr&~1] = v; return; }
		*(u32*)&oam[addr&~3] = v; return;	
	}
} //end of write

u32 gba::read(u32 addr, int size, ARM_CYCLE ct)
{
	cpu.icycles += 1;
	if( addr < 0x0800'0000 ) printf("GBA: Read $%X\n", addr);
	if( addr < 0x0200'0000 )
	{
		if( cpu.r[15] > 0x4008 ) return 0;
		if( size == 8 ) return bios[addr];
		if( size == 16 ) return *(u16*)&bios[addr&~1];
		return *(u32*)&bios[addr&~3];
	}
	if( addr < 0x0300'0000 )
	{
		addr &= 0x3ffff;
		if( size == 8 ) return ewram[addr];
		if( size == 16 ) return *(u16*)&ewram[addr&~1];
		return *(u32*)&ewram[addr&~3];
	}
	if( addr < 0x0400'0000 )
	{
		addr &= 0x7fff;
		if( size == 8 ) return iwram[addr];
		if( size == 16 ) return *(u16*)&iwram[addr&~1];
		return *(u32*)&iwram[addr&~3];
	}
	if( addr < 0x0500'0000 )
	{
		if( addr == 0x0400'0130 ) return getKeys();
		if( addr == 0x0400'0004 )
		{
			u32 r = dispstat;
			dispstat ^= 1;
			return r;
		}
		printf("IO Read $%X\n", addr);		
		return 0;
	}
	if( addr < 0x0600'0000 )
	{
		addr &= 0x3ff;
		if( size == 8 ) return palette[addr];
		if( size == 16 ) return *(u16*)&palette[addr&~1];
		return *(u32*)&palette[addr&~3];
	}
	if( addr < 0x0700'0000 )
	{
		addr &= 0x1ffff;
		if( size == 8 ) return vram[addr];
		if( size == 16 ) return *(u16*)&vram[addr&~1];
		return *(u32*)&vram[addr&~3];
	}
	if( addr < 0x0800'0000 )
	{
		addr &= 0x3ff;
		if( size == 8 ) return oam[addr];
		if( size == 16 ) return *(u16*)&oam[addr&~1];
		return *(u32*)&oam[addr&~3];	
	}
	if( addr < (0x0800'0000 + ROM.size()) )
	{
		addr -= 0x0800'0000;
		if( size == 8 ) return ROM[addr];
		if( size == 16) return *(u16*)&ROM[addr&~1];
		return *(u32*)&ROM[addr&~3];
	}
	
	printf("GBA:$%X: Unimpl. read $%X\n", cpu.r[15]-(cpu.cpsr.b.T?4:8), addr);
	exit(1);
}

void gba::reset()
{
	cpu.read = [](u32 a, int s, ARM_CYCLE c) -> u32 { return dynamic_cast<gba*>(sys)->read(a,s,c); };
	cpu.write=[](u32 a, u32 v, int s, ARM_CYCLE c) { dynamic_cast<gba*>(sys)->write(a,v,s,c); };
	cpu.reset();
}

void gba::run_frame()
{
	char c;
	std::cout << "Debug action? ";
	std::cin >> c;	
	if( c == 'n' )
	{
		printf("lpc = $%X\n", cpu.r[15] - (cpu.cpsr.b.T?2:4));
		stamp += cpu.step();
		cpu.dump_regs();
	} else if( c == 'p' ) {
		int addr;
		std::cin >> std::hex >> addr;
		std::cout << cpu.read(addr, 32, ARM_CYCLE::X) << std::endl;
	} else if( c == 'r' ) {
		u32 addr;
		std::cin >> std::hex >> addr;
		while( cpu.r[15] != addr ) stamp += cpu.step();
		cpu.dump_regs();
	}
		
	if( stamp < cycles_per_frame ) return;
	stamp -= cycles_per_frame;
	
	//while( stamp < cycles_per_frame ) stamp += cpu.step();
	//stamp -= cycles_per_frame;
	//memcpy(fbuf, vram, 240*160*2);
	if(1) for(u32 y = 0; y < 160; ++y)
	{
		for(u32 x = 0; x < 240; ++x)
		{
			fbuf[y*240+x] = *(u16*)&palette[vram[y*240+x]<<1];		
		}
	}
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


