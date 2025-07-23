#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include "gba.h"

#define DISPSTAT lcd.regs[2]
#define VCOUNT lcd.regs[3]
const u32 cycles_per_frame = 280896;

static void sized_write(u8* data, u32 addr, u32 v, int size)
{
	if( size == 8 ) data[addr] = v; 
	else if( size == 16 ) *(u16*)&data[addr] = v;
	else *(u32*)&data[addr] = v;
}

static u32 sized_read(u8* data, u32 addr, int size)
{
	if( size == 8 ) return data[addr]; 
	else if( size == 16 ) return *(u16*)&data[addr];
	else return *(u32*)&data[addr];
}

void gba::write(u32 addr, u32 v, int size, ARM_CYCLE ct)
{
	if( addr < 0x02000000 ) { return; } // write to BIOS ignored
	if( addr < 0x03000000 )
	{
		addr &= 0x3FFFF;
		sized_write(ewram, addr, v, size);
		return;
	}
	if( addr < 0x04000000 )
	{
		addr &= 0x7fff;
		sized_write(iwram, addr, v, size);
		return;
	}
	if( addr < 0x05000000 )
	{  // I/O writes
		write_io(addr, v, size);
		//std::println("IO Write{} ${:X} = ${:X}", size, addr, v);
		return;
	}
	if( addr < 0x06000000 )
	{
		addr &= 0x3ff;
		if( size == 8 ) { size = 16; v = (v&0xff)*0x101; addr &= ~1; }
		sized_write(palette, addr, v, size);
		return;
	}
	if( addr < 0x07000000 )
	{ // VRAM todo mirroring
	
		if( size == 8 )
		{
			size = 16;
			addr &= ~1;
			v &= 0xff;
			v |= v<<8;
		}
		addr &= 0x1ffff;
		if( addr & 0x10000 ) addr &= ~0x8000;
		sized_write(vram, addr&0x7ffff, v, size);
		return;
	}
	if( addr < 0x08000000 )
	{ // OAM
		sized_write(oam, addr&0x3ff, v, size);
		return;
	}
	std::println("${:X} = ${:X}", addr, v);
} //end of write

u32 gba::read(u32 addr, int size, ARM_CYCLE ct)
{
	if( addr >= 0x08000000 ) 
	{
		if( addr >= 0x0d000000 && addr  < 0x0e000000 ) return 1; 
		//^stubs some kind of save hw, yoinked from older emu to get vrally3 going
	
		addr -= 0x08000000u;
		if( addr >= ROM.size() )
		{
			//std::println("${:X}:{} Read from beyond rom at ${:X}", cpu.r[15]-4, (u32)cpu.cpsr.b.T, addr);
			//exit(1);
			return 0;
		}
		return sized_read(ROM.data(), addr, size);
	}
	if( addr < 0x02000000 ) 
	{ 
		if( cpu.r[15]-4 < 0x4000 ) 
			return sized_read(bios, addr&0x3fff, size);
		return cpu.fetch; 
	}
	if( addr < 0x03000000 )
	{
		addr &= 0x3FFFF;
		return sized_read(ewram, addr, size);
	}
	if( addr < 0x04000000 )
	{
		addr &= 0x7fff;
		return sized_read(iwram, addr, size);
	}
	if( addr < 0x05000000 )
	{  // I/O
		if( addr == 0x04000130 ) { return getKeys(); }
		return read_io(addr, size);
	}
	if( addr < 0x06000000 )
	{
		addr &= 0x3ff;
		return sized_read(palette, addr, size);
	}
	if( addr < 0x07000000 )
	{ // VRAM
		addr &= 0x1ffff;
		if( addr & 0x10000 ) addr &= ~0x8000;
		return sized_read(vram, addr, size);
	}
	if( addr < 0x08000000 )
	{ // OAM
		return sized_read(oam, addr&0x3ff, size);
	}
	return 0;
}

void gba::reset()
{
	cpu.read = [&](u32 a, int s, ARM_CYCLE c) -> u32 { return read(a,s,c); };
	cpu.write= [&](u32 a, u32 v, int s, ARM_CYCLE c) { write(a,v,s,c); };
	cpu.reset();
	target_stamp = 0;
	halted = false;
	
	lcd.regs[0] = lcd.regs[2] = 0;
	ISTAT = IMASK = IME = 0;
		
	memset(vram, 0, 96*1024);
}

gba::gba() : lcd(vram, palette, oam)
{
}

void gba::run_frame()
{
	VCOUNT = 0;
	while( VCOUNT < 228 )
	{
		DISPSTAT &= ~1;
		DISPSTAT |= (VCOUNT >= 160 && VCOUNT != 228);
		if( VCOUNT == 160 && (DISPSTAT & BIT(3)) )
		{
			ISTAT |= BIT(0);
			check_irqs();
		}
		if( VCOUNT == (DISPSTAT>>8) )
		{
			DISPSTAT |= 4;
			if( DISPSTAT & BIT(5) )
			{
				ISTAT |= BIT(2);
				check_irqs();
			}
		} else {
			DISPSTAT &= ~4;
		}
		if( DISPSTAT & BIT(4) )
		{
			ISTAT |= BIT(1);
			check_irqs();
		}
		
		if( halted )
		{
			cpu.stamp = target_stamp;
		}
		while( cpu.stamp < target_stamp )
		{
			cpu.step();
		}
		target_stamp += 1232;
		
		if( VCOUNT < 160 ) lcd.draw_scanline(VCOUNT);
		VCOUNT += 1;
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

void gba::check_irqs()
{
	cpu.irq_line = IME && (ISTAT & IMASK & 0x3fff);
	if( halted ) halted = !(ISTAT & IMASK & 0x3fff);
}

