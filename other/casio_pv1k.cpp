#include <cstdio>
#include <cstdlib>
#include "casio_pv1k.h"
#include "z80.h"
extern console* sys;

void pv1k::write(u16 addr, u8 val)
{
	if( addr >= 0x8000 ) RAM[addr&0x7ff] = val;
}

u8 pv1k::read(u16 addr)
{
	if( addr >= 0x8000 ) return RAM[addr&0x7ff];
	return ROM[addr%ROM.size()];
}

void pv1k::out(u16 port, u8 val)
{
	port &= 0xff;
	if( port == 0xFD )
	{
		fd_select = val;
		return;
	}
	
	if( port == 0xFE )
	{
		ram_taddr = val;
		return;
	}
	
	if( port == 0xFF )
	{
		rom_taddr = val;
		return;
	}
}

u8 pv1k::in(u16 port)
{
	port &= 0xff;
	
	if( port == 0xFC )
	{
		u8 res = (cpu.irq_line)?1:0;
		cpu.irq_line = 0;
		return res|2;	
	}
	
	if( port == 0xFD )
	{
		u8 res = 0;
		auto keys = SDL_GetKeyboardState(nullptr);
		if( fd_select & 1 )
		{
			if( keys[SDL_SCANCODE_A] ) res |= 1;
			if( keys[SDL_SCANCODE_S] ) res |= 2;		
		}
		if( fd_select & 2 )
		{
			if( keys[SDL_SCANCODE_DOWN] ) res |= 1;
			if( keys[SDL_SCANCODE_RIGHT] ) res |= 2;		
		}
		if( fd_select & 4 )
		{
			if( keys[SDL_SCANCODE_LEFT] ) res |= 1;
			if( keys[SDL_SCANCODE_UP] ) res |= 2;		
		}
		if( fd_select & 4 )
		{
			if( keys[SDL_SCANCODE_Z] ) res |= 1;
			if( keys[SDL_SCANCODE_X] ) res |= 2;		
		}
		return res|0x80;
	}
	
	return 0;
}

bool pv1k::loadROM(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if(!fp) return false;
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if( fsz == 0 ) return false;
	
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	
	return true;
}

void pv1k_write(u16 a, u8 v) { dynamic_cast<pv1k*>(sys)->write(a,v); }
u8 pv1k_read(u16 a) { return dynamic_cast<pv1k*>(sys)->read(a); }
u8 pv1k_in(u16 p) { return dynamic_cast<pv1k*>(sys)->in(p); }
void pv1k_out(u16 p, u8 v) { dynamic_cast<pv1k*>(sys)->out(p,v); }

void pv1k::reset()
{
	memset(&cpu, 0, sizeof(cpu));
	cpu.reset();
	cpu.read = pv1k_read;
	cpu.write = pv1k_write;
	cpu.in = pv1k_in;
	cpu.out = pv1k_out;
	fbuf.clear(); fbuf.resize(fb_width()*fb_height());
	stamp = last_target = 0;
}

void pv1k::scanline(u32 line)
{
	for(u32 X = 0; X < 240; ++X)
	{
		u8 tile_idx = RAM[2 + (X/8) + (line/8)*32];
		u8* tile_data;
		if( tile_idx < 0xE0 || (rom_taddr&0x10) )
		{
		    tile_data = &ROM[(rom_taddr>>5)*0x2000 + tile_idx*32];
		} else {
		    tile_idx &= 0x1F;
		    tile_data = &RAM[((((ram_taddr>>2)|3)<<10) + tile_idx*32)&0x7ff];
		}
	
		u8 R = tile_data[8 + (line&7)] & (1u<<(7^(X&7)));
		u8 G = tile_data[16+ (line&7)] & (1u<<(7^(X&7)));
		u8 B = tile_data[24+ (line&7)] & (1u<<(7^(X&7)));
		R = R ? 0xff : 0;
		G = G ? 0xff : 0;
		B = B ? 0xff : 0;
		fbuf[line*fb_width() + X] = (R<<24)|(G<<16)|(B<<8);
	}
}

void pv1k::run_frame()
{
	for(u32 line = 0; line < 262; ++line)
	{
		if( line >= 196 && line <= 256 && (line&3) == 0 ) cpu.irq_line = 1;	
		const u64 target = last_target + 227;
		while( stamp < target )
		{
			const u64 cycles = cpu.step();
			stamp += cycles;
		}
		last_target = target;
		if( line < fb_height() ) scanline(line);
	}
}

